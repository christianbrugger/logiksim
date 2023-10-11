#ifndef LOGICSIM_LOGIC_ITEM_LAYOUT_STANDARD_ELEMENT_H
#define LOGICSIM_LOGIC_ITEM_LAYOUT_STANDARD_ELEMENT_H

#include "vocabulary/connection_count.h"
#include "vocabulary/grid.h"

namespace logicsim {

namespace standard_element {

constexpr static inline auto min_inputs = connection_count_t {2};
constexpr static inline auto max_inputs = connection_count_t {128};
constexpr static inline auto default_inputs = connection_count_t {2};

constexpr static inline auto width = grid_t {2};
[[nodiscard]] auto height(connection_count_t input_count) -> grid_t;

}  // namespace standard_element

}  // namespace logicsim

#endif
