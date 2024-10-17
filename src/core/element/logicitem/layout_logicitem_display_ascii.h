#ifndef LOGICSIM_LOGIC_ITEM_LAYOUT_DISPLAY_ASCII_H
#define LOGICSIM_LOGIC_ITEM_LAYOUT_DISPLAY_ASCII_H

#include "core/algorithm/range.h"
#include "core/element/logicitem/layout_logicitem_display.h"
#include "core/geometry/connection_count.h"
#include "core/vocabulary/connection_count.h"
#include "core/vocabulary/connection_id.h"
#include "core/vocabulary/connector_info.h"
#include "core/vocabulary/grid.h"
#include "core/vocabulary/layout_info_vector.h"
#include "core/vocabulary/orientation.h"
#include "core/vocabulary/point.h"

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
constexpr static inline auto static_inputs = []() -> static_inputs_t {
    auto inputs = static_inputs_t {};

    // enable as first input
    static_assert(display::enable_input_id == connection_id_t {0});
    inputs.push_back({display_ascii::enable_position, orientation_t::down});

    // number inputs
    for (auto y : range(to_grid(value_inputs))) {
        inputs.push_back({point_t {0, y}, orientation_t::left});
    }

    return inputs;
}();

}  // namespace display_ascii

}  // namespace logicsim

#endif
