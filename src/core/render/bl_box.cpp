#include "core/render/bl_box.h"

namespace logicsim {

auto is_box_non_empty(const BLBox &a) -> bool {
    return (a.x1 > a.x0) || (a.y1 > a.y0);
}

auto is_box_empty(const BLBox &a) -> bool {
    return !is_box_non_empty(a);
}

auto get_box_union(const BLBox &a, const BLBox &b) -> BLBox {
    return BLBox {
        std::min(a.x0, b.x0),
        std::min(a.y0, b.y0),
        std::max(a.x1, b.x1),
        std::max(a.y1, b.y1),
    };
}

}  // namespace logicsim
