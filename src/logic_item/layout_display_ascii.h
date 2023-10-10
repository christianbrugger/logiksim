#ifndef LOGICSIM_LOGIC_ITEM_LAYOUT_DISPLAY_ASCII_H
#define LOGICSIM_LOGIC_ITEM_LAYOUT_DISPLAY_ASCII_H

#include "geometry/connection_count.h"
#include "vocabulary/connection_count.h"
#include "vocabulary/grid.h"
#include "vocabulary/point.h"

namespace logicsim {


namespace display_ascii {

constexpr static inline auto control_inputs = connection_count_t {1};
constexpr static inline auto value_inputs = connection_count_t {7};
constexpr static inline auto input_count = control_inputs + value_inputs;

constexpr static inline auto width = grid_t {4};
constexpr static inline auto height = to_grid(value_inputs - connection_count_t {1});
constexpr static inline auto enable_position = point_t {2, height};

}  // namespace display_ascii

}  // namespace logicsim

#endif
