#ifndef LOGICSIM_CORE_IMAGE_SURFACE_H
#define LOGICSIM_CORE_IMAGE_SURFACE_H

#include "render/context.h"
#include "render/context_guard.h"
#include "render/image.h"
#include "render/render_context.h"

#include <blend2d.h>

#include <concepts>

namespace logicsim {

/**
 * @brief: A Render Context that own the target image.
 */
class ImageSurface {
   public:
    [[nodiscard]] auto bl_image() const -> const BLImage &;

    /**
     * @brief: Renders the given function in the stored bl_image.
     *
     * Automatically resizes the bl_image as needed by the settings.
     */
    template <std::invocable<Context &> Func>
    inline auto render(const ContextRenderSettings &settings, ContextCache cache,
                       Func render_function) -> void;

   private:
    BLImage bl_image_ {};
};

/**
 * @brief: Copies the data from the source layer to the target context.
 *
 * Throws if source and target don't have the same size.
 */
auto blit_layer(Context &target_ctx, const ImageSurface &source_layer,
                BLRectI dirty_rect) -> void;

/**
 * @brief: Renders the function first to the layer and then to the target within
 *         the given dirty_rect.
 */
template <std::invocable<Context &> Func>
auto render_layer(Context &target_ctx, ImageSurface &layer, BLRectI dirty_rect,
                  Func render_func) -> void;

//
// Implementation
//

template <std::invocable<Context &> Func>
inline auto ImageSurface::render(const ContextRenderSettings &settings,
                                 ContextCache cache, Func render_function) -> void {
    resize_image_no_copy(bl_image_, settings.view_config.size());
    render_to_image(bl_image_, settings, std::move(cache), std::move(render_function));
}

template <std::invocable<Context &> Func>
auto render_layer(Context &target_ctx, ImageSurface &layer, BLRectI dirty_rect,
                  Func render_func) -> void {
    layer.render(target_ctx.settings, target_ctx.cache, [&](Context &layer_ctx) {
        layer_ctx.bl_ctx.clearRect(dirty_rect);
        auto _ [[maybe_unused]] = make_context_guard(layer_ctx);
        std::invoke(render_func, layer_ctx);
    });

    blit_layer(target_ctx, layer, dirty_rect);
}

}  // namespace logicsim

#endif
