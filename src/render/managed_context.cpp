#include "render/managed_context.h"

#include "format/blend2d_type.h"  // TODO remove
#include "logging.h"              // TODO remove
#include "render/bl_error_check.h"
#include "render/context_info.h"

#include <gsl/gsl>

#include <stdexcept>

namespace logicsim {

auto create_context(BLImage &bl_image, const ContextRenderSettings &render_settings)
    -> BLContext {
    if (bl_image.size() != render_settings.view_config.size()) {
        throw std::runtime_error("Given bl_image does not match size of settings.");
    }

    return BLContext {bl_image, context_info(render_settings)};
}

auto resize_image_no_copy(BLImage &image, BLSizeI new_size) -> void {
    if (image.size() != new_size) {
        if (image.create(new_size.w, new_size.h, BL_FORMAT_PRGB32) != BL_SUCCESS) {
            throw std::runtime_error("Error while calling BLImage::create");
        }
    }
}

//
// Image Context
//

auto ImageSurface::bl_image() const -> const BLImage & {
    return bl_image_;
}

//
// Free Functions
//

auto blit_layer(Context &target_ctx, const BLImage &source_image, BLRectI dirty_rect)
    -> void {
    if (target_ctx.bl_ctx.targetSize() != source_image.size()) [[unlikely]] {
        throw std::runtime_error("target_ctx and source_image need to have same size.");
    }
    // TODO ??? throws if dirty_rect is not within size
    // print("dirty_rect", dirty_rect);

    auto _ [[maybe_unused]] = make_context_guard(target_ctx);

    target_ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_OVER);
    target_ctx.bl_ctx.blitImage(dirty_rect, source_image, dirty_rect);
}

auto blit_layer(Context &target_ctx, const ImageSurface &source_layer, BLRectI dirty_rect)
    -> void {
    blit_layer(target_ctx, source_layer.bl_image(), dirty_rect);
}

}  // namespace logicsim
