#ifndef LOGICSIM_LOGIC_ITEM_LAYOUT_DISPLAY_ASCII_H
#define LOGICSIM_LOGIC_ITEM_LAYOUT_DISPLAY_ASCII_H

#include "algorithm/range.h"
#include "geometry/connection_count.h"
#include "logic_item/layout_display.h"
#include "vocabulary/connection_count.h"
#include "vocabulary/connection_id.h"
#include "vocabulary/connector_info.h"
#include "vocabulary/grid.h"
#include "vocabulary/layout_info_small_vector.h"
#include "vocabulary/orientation.h"
#include "vocabulary/point.h"

namespace logicsim {

namespace display_ascii {

constexpr static inline auto control_inputs = connection_count_t {1};
constexpr static inline auto value_inputs = connection_count_t {7};
constexpr static inline auto input_count = control_inputs + value_inputs;

constexpr static inline auto width = grid_t {4};
constexpr static inline auto height = to_grid(value_inputs - connection_count_t {1});
constexpr static inline auto enable_position = point_t {2, height};

/**
 * @brief: List of static input connectors.
 */
constexpr static inline auto input_connectors = []() -> static_connectors {
    auto inputs = static_connectors {};

    // enable as first input
    static_assert(display::enable_input_id == connection_id_t {0});
    inputs.push_back({display_ascii::enable_position, orientation_t::down});

    // number inputs
    for (auto y : range(to_grid(value_inputs))) {
        inputs.push_back(connector_info_t {point_t {0, y}, orientation_t::left});
    }

    return inputs;
}();

}  // namespace display_ascii

}  // namespace logicsim

#endif
