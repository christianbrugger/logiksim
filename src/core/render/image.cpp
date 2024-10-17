#include "core/render/image.h"

#include <gsl/gsl>

namespace logicsim {

auto resize_image_no_copy(BLImage &image, BLSizeI new_size) -> void {
    if (image.size() != new_size) {
        Expects(image.create(new_size.w, new_size.h, BL_FORMAT_PRGB32) == BL_SUCCESS);
    }
}

}  // namespace logicsim
