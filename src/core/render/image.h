#ifndef LOGICSIM_CORE_RENDER_IMAGE_H
#define LOGICSIM_CORE_RENDER_IMAGE_H

#include <blend2d.h>

namespace logicsim {

/**
 * @brief: Allocates a new image if the size is different, without copying data.
 */
auto resize_image_no_copy(BLImage &image, BLSizeI new_size) -> void;

}  // namespace logicsim

#endif
