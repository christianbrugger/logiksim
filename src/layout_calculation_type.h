#ifndef LOGIKSIM_LAYOUT_CALCULATION_TYPE
#define LOGIKSIM_LAYOUT_CALCULATION_TYPE

#include "line_tree.h"
#include "vocabulary.h"

namespace logicsim {

struct layout_calculation_data_t {
    const LineTree &line_tree;
    std::size_t input_count;
    std::size_t output_count;
    std::size_t internal_state_count;
    point_t position;
    orientation_t orientation;
    ElementType element_type;
};

constexpr auto is_placeholder(const layout_calculation_data_t &data) -> bool {
    return data.element_type == ElementType::placeholder;
}

}  // namespace logicsim

#endif
