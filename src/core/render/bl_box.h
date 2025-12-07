#ifndef LOGICSIM_CORE_RENDER_BL_BOX_H
#define LOGICSIM_CORE_RENDER_BL_BOX_H

#include "core/concept/input_range.h"

#include <blend2d/blend2d.h>
#include <range/v3/numeric/accumulate.hpp>

#include <limits>

namespace logicsim {

constexpr static inline auto empty_bl_box = BLBox {
    +std::numeric_limits<double>::infinity(),
    +std::numeric_limits<double>::infinity(),
    -std::numeric_limits<double>::infinity(),
    -std::numeric_limits<double>::infinity(),
};

[[nodiscard]] auto is_box_non_empty(const BLBox &a) -> bool;
[[nodiscard]] auto is_box_empty(const BLBox &a) -> bool;

[[nodiscard]] auto get_box_union(const BLBox &a, const BLBox &b) -> BLBox;

[[nodiscard]] auto get_box_union(input_range_of<BLBox> auto &&boxes) -> BLBox {
    return ranges::accumulate(boxes, empty_bl_box, [](const BLBox &a, const BLBox &b) {
        return get_box_union(a, b);
    });
}

}  // namespace logicsim

#endif
