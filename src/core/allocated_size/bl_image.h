#ifndef LOGICSIM_CORE_ALLOCATED_SIZE_BL_IMAGE_H
#define LOGICSIM_CORE_ALLOCATED_SIZE_BL_IMAGE_H

#include "core/algorithm/numeric.h"
#include "core/allocated_size/trait.h"

#include <blend2d.h>
#include <gsl/gsl>

namespace logicsim {

template <>
struct AllocatedSizeTrait<BLImage> {
    [[nodiscard]] static auto allocated_size(const BLImage& bl_image) -> std::size_t {
        auto data = BLImageData {};
        bl_image.getData(&data);

        const auto height = gsl::narrow<std::size_t>(data.size.h);
        const auto stride = gsl::narrow<std::size_t>(data.stride);
        return checked_mul(height, stride);
    }
};

}  // namespace logicsim

#endif
