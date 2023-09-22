#ifndef LOGIKSIM_LAYOUT_CALCULATION_TYPE_H
#define LOGIKSIM_LAYOUT_CALCULATION_TYPE_H

#include "vocabulary.h"

namespace logicsim {

class Circuit;
class Schematic;
class Layout;
class SegmentTree;

struct layout_calculation_data_t {
    std::size_t input_count {0};
    std::size_t output_count {0};
    std::size_t internal_state_count {0};
    point_t position {0, 0};
    orientation_t orientation {orientation_t::undirected};
    ElementType element_type {ElementType::placeholder};

    [[nodiscard]] auto operator==(const layout_calculation_data_t& other) const
        -> bool = default;
};

static_assert(sizeof(layout_calculation_data_t) == 32);

[[nodiscard]] constexpr auto is_placeholder(const layout_calculation_data_t& data)
    -> bool {
    return data.element_type == ElementType::placeholder;
}


}  // namespace logicsim

#endif
