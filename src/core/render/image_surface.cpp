#include "render/image_surface.h"

namespace logicsim {

//
// Image Context
//

auto ImageSurface::bl_image() const -> const BLImage & {
    return bl_image_;
}

//
// Free Functions
//

auto blit_layer(Context &target_ctx, const ImageSurface &source_layer,
                BLRectI dirty_rect) -> void {
    blit_layer(target_ctx, source_layer.bl_image(), dirty_rect);
}

}  // namespace logicsim
