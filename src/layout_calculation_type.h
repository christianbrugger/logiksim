#ifndef LOGIKSIM_LAYOUT_CALCULATION_TYPE
#define LOGIKSIM_LAYOUT_CALCULATION_TYPE

#include "vocabulary.h"

namespace logicsim {

class Circuit;
class Schematic;
class Layout;
class SegmentTree;

struct layout_calculation_data_t {
    const SegmentTree& segment_tree;
    std::size_t input_count {0};
    std::size_t output_count {0};
    std::size_t internal_state_count {0};
    point_t position {0, 0};
    orientation_t orientation {orientation_t::undirected};
    ElementType element_type {ElementType::placeholder};
};

static_assert(sizeof(layout_calculation_data_t) == 40);

[[nodiscard]] constexpr auto is_placeholder(const layout_calculation_data_t& data)
    -> bool {
    return data.element_type == ElementType::placeholder;
}

[[nodiscard]] auto to_layout_calculation_data(const Schematic& schematic,
                                              const Layout& layout,
                                              element_id_t element_id)
    -> layout_calculation_data_t;

[[nodiscard]] auto to_layout_calculation_data(const Circuit& circuit,
                                              element_id_t element_id)
    -> layout_calculation_data_t;

}  // namespace logicsim

#endif
