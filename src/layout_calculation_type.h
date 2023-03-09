#ifndef LOGIKSIM_LAYOUT_CALCULATION_TYPE
#define LOGIKSIM_LAYOUT_CALCULATION_TYPE

#include "line_tree.h"
#include "vocabulary.h"

namespace logicsim {

class Schematic;
class Layout;

struct layout_calculation_data_t {
    const LineTree& line_tree;
    std::size_t input_count {0};
    std::size_t output_count {0};
    std::size_t internal_state_count {0};
    point_t position {0, 0};
    orientation_t orientation {orientation_t::undirected};
    ElementType element_type {ElementType::placeholder};
};

constexpr auto is_placeholder(const layout_calculation_data_t& data) -> bool {
    return data.element_type == ElementType::placeholder;
}

auto to_layout_calculation_data(const Schematic& schematic, const Layout& layout,
                                element_id_t element_id) -> layout_calculation_data_t;

}  // namespace logicsim

#endif