#ifndef LOGICSIM_CORE_ALLOCATED_SIZE_BL_IMAGE_H
#define LOGICSIM_CORE_ALLOCATED_SIZE_BL_IMAGE_H

#include "core/allocated_size/trait.h"

#include <blend2d.h>

namespace logicsim {

template <>
struct AllocatedSizeTrait<BLImage> {
    [[nodiscard]] static auto allocated_size(const BLImage& bl_image) -> std::size_t {
        auto data = BLImageData {};
        bl_image.getData(&data);
        return data.size.h * data.stride;
    }
};

}  // namespace logicsim

#endif
