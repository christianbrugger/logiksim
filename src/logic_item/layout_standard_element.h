#ifndef LOGICSIM_LOGIC_ITEM_LAYOUT_STANDARD_ELEMENT_H
#define LOGICSIM_LOGIC_ITEM_LAYOUT_STANDARD_ELEMENT_H

#include "algorithm/range.h"
#include "geometry/connection_count.h"
#include "vocabulary/connection_count.h"
#include "vocabulary/grid.h"
#include "vocabulary/layout_calculation_data.h"
#include "vocabulary/layout_info_small_vector.h"
#include "vocabulary/orientation.h"
#include "vocabulary/point.h"

#include <concepts>

namespace logicsim {

namespace standard_element {

constexpr static inline auto min_inputs = connection_count_t {2};
constexpr static inline auto max_inputs = connection_count_t {128};
constexpr static inline auto default_inputs = connection_count_t {2};

constexpr static inline auto width = grid_t {2};

/**
 * @brief: Returns the dynamic height of the standard element.
 */
[[nodiscard]] constexpr inline auto height(connection_count_t input_count) -> grid_t {
    return to_grid((input_count - connection_count_t {1}));
}

/**
 * @brief: Returns the dynamic y-coordinate of the output of the standard element.
 */
[[nodiscard]] constexpr inline auto output_height(connection_count_t input_count)
    -> grid_t {
    return height(input_count) / 2;
}

/**
 * @brief: Iterate over the inputs of standard elements
 *         not considering position or orientation.
 *
 *  next_input = [](point_t position, orientation_t orientation) -> bool
 *
 * The callable is called for each point or until it returns false.
 */
constexpr inline auto iter_input_location(
    const layout_calculation_data_t& data,
    std::invocable<point_t, orientation_t> auto next_input) -> bool {
    for (auto y : range(to_grid(data.input_count))) {
        if (!next_input(point_t {0, y}, orientation_t::left)) {
            return false;
        }
    }
    return true;
}

/**
 * @brief: Iterate over the outputs of standard elements
 *         not considering position or orientation.
 *
 *  next_output = [](point_t position, orientation_t orientation) -> bool
 *
 * The callable is called for each point or until it returns false.
 */
constexpr inline auto iter_output_location(
    const layout_calculation_data_t& data,
    std::invocable<point_t, orientation_t> auto next_output) -> bool {
    const auto output_y = output_height(data.input_count);

    if (!next_output(point_t {2, output_y}, orientation_t::right)) {
        return false;
    }
    return true;
}

/**
 * @brief: Iterate over the body points of standard elements
 *         not considering position or orientation.
 *
 *  next_point = [](point_t position) -> bool
 *
 * The callable is called for each point or until it returns false.
 */
constexpr inline auto iter_element_body_points(const layout_calculation_data_t& data,
                                               std::invocable<point_t> auto next_point)
    -> bool {
    const auto output_y = output_height(data.input_count);

    for (auto y : range(to_grid(data.input_count))) {
        if (!next_point(point_t {1, y})) {
            return false;
        }

        if (y != output_y) {
            if (!next_point(point_t {2, y})) {
                return false;
            }
        }
    }
    return true;
}

[[nodiscard]] auto iter_element_body_points(const layout_calculation_data_t& data)
    -> body_points_vector;

}  // namespace standard_element

}  // namespace logicsim

#endif
