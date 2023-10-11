#ifndef LOGICSIM_LOGIC_ITEM_LAYOUT_DISPLAY_NUMBER_H
#define LOGICSIM_LOGIC_ITEM_LAYOUT_DISPLAY_NUMBER_H

#include "vocabulary/connection_count.h"
#include "vocabulary/grid.h"
#include "vocabulary/layout_calculation_data.h"
#include "vocabulary/orientation.h"
#include "vocabulary/point.h"

#include <concepts>

namespace logicsim {

namespace display_number {

constexpr static inline auto control_inputs = connection_count_t {2};
constexpr static inline auto min_value_inputs = connection_count_t {1};
constexpr static inline auto max_value_inputs = connection_count_t {64};
constexpr static inline auto default_value_inputs = connection_count_t {3};

constexpr static inline auto min_inputs = control_inputs + min_value_inputs;
constexpr static inline auto max_inputs = control_inputs + max_value_inputs;
constexpr static inline auto default_inputs = control_inputs + default_value_inputs;

[[nodiscard]] auto value_inputs(connection_count_t input_count) -> connection_count_t;
[[nodiscard]] auto width(connection_count_t input_count) -> grid_t;
[[nodiscard]] auto height(connection_count_t input_count) -> grid_t;

[[nodiscard]] auto input_shift(connection_count_t input_count) -> grid_t;
[[nodiscard]] auto enable_position(connection_count_t input_count) -> point_t;
[[nodiscard]] auto negative_position(connection_count_t input_count) -> point_t;
constexpr static inline auto negative_input_id = connection_id_t {1};

//
// Implementation
//

// next_input = [](point_t position, orientation_t orientation) -> bool
auto iter_input_location(const layout_calculation_data_t& data,
                         std::invocable<point_t, orientation_t> auto next_input) -> bool {
    // for (auto y : range(to_grid(data.input_count))) {
    //     if (!next_input(point_t {0, y}, orientation_t::left)) {
    //         return false;
    //     }
    // }
    // return true;
}

// next_output = [](point_t position, orientation_t orientation) -> bool
auto iter_output_location(const layout_calculation_data_t& data,
                          std::invocable<point_t, orientation_t> auto next_output)
    -> bool {
    // const auto output_y = output_height(data.input_count);

    // if (!next_output(point_t {2, output_y}, orientation_t::left)) {
    //     return false;
    // }
}

// next_point = [](point_t position) -> bool
auto iter_element_body_points(const layout_calculation_data_t& data,
                              std::invocable<point_t> auto next_point) -> bool {
    // const auto output_y = output_height(data.input_count);

    // for (auto y : range(height(data.input_count))) {
    //     if (!next_point(point_t {1, y})) {
    //         return false;
    //     }

    //    if (y != output_y) {
    //        if (!next_point(point_t {2, y})) {
    //            return false;
    //        }
    //    }
    //}
    // return true;
}

}  // namespace display_number

}  // namespace logicsim

#endif
