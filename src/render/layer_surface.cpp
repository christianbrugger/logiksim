#include "render/layer_surface.h"

#include "render/context_guard.h"
#include "render/context_info.h"

namespace logicsim {

auto LayerSurface::initialize(const ContextRenderSettings& new_settings) -> void {
    const auto new_size = new_settings.view_config.size();

    // image size changed
    if (ctx.bl_image.size() != new_size) {
        ctx.end();
        ctx.settings = new_settings;
        ctx.bl_image = BLImage {new_size.w, new_size.h, BL_FORMAT_PRGB32};
        ctx.begin();
    }

    // context info changed
    else if (!equals(context_info(ctx.settings), context_info(new_settings))) {
        ctx.end();
        ctx.settings = new_settings;
        ctx.begin();
    }

    // only settings update
    else {
        ctx.settings = new_settings;
    }
}

auto LayerSurface::clear() -> void {
    ctx.clear();
}

auto LayerSurface::shrink_to_fit() -> void {
    ctx.shrink_to_fit();
}

auto render_to_layer(Context& target_ctx, LayerSurface& surface, BLRectI dirty_rect,
                     std::function<void(Context&, bool)> render_func) -> void {
    auto _ [[maybe_unused]] = make_context_guard(target_ctx);

    if (surface.enabled) {
        surface.initialize(target_ctx.settings);
        surface.ctx.bl_ctx.clearRect(dirty_rect);

        {
            auto __ [[maybe_unused]] = make_context_guard(surface);
            render_func(surface.ctx, surface.enabled);
        }

        surface.ctx.sync();
        target_ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_OVER);
        target_ctx.bl_ctx.blitImage(dirty_rect, surface.ctx.bl_image, dirty_rect);
    } else {
        render_func(target_ctx, surface.enabled);
    }
}

[[nodiscard]] auto make_context_guard(LayerSurface& surface) -> ContextGuard {
    return make_context_guard(surface.ctx);
}

}  // namespace logicsim
