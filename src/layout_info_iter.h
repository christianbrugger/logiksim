#ifndef LOGICSIM_LAYOUT_INFO_ITER_H
#define LOGICSIM_LAYOUT_INFO_ITER_H

#include "geometry/layout_calculation.h"
#include "logic_item/layout.h"
#include "vocabulary/connection_id.h"
#include "vocabulary/layout_calculation_data.h"
#include "vocabulary/orientation.h"
#include "vocabulary/point.h"

#include <concepts>

namespace logicsim {

/**
 * @brief: Iterate the input locations.
 *
 *  next_input = [](point_t position, orientation_t orientation) -> bool
 *
 * The callable is called for each point or until it returns false.
 *
 * Returns the value of the last callable result.
 */
inline auto iter_input_location(const layout_calculation_data_t &data,
                                std::invocable<point_t, orientation_t> auto next_input)
    -> bool;

/**
 * @brief: Iterate the outputs locations.
 *
 *  next_output = [](point_t position, orientation_t orientation) -> bool
 *
 * The callable is called for each point or until it returns false.
 *
 * Returns the value of the last callable result.
 */
inline auto iter_output_location(const layout_calculation_data_t &data,
                                 std::invocable<point_t, orientation_t> auto next_output)
    -> bool;

/**
 * @brief: Iterate over the body points not considering position or orientation.
 *
 *  next_point = [](point_t position) -> bool
 *
 * The callable is called for each point or until it returns false.
 *
 * Returns the value of the last callable result.
 */
inline auto iter_element_body_points(const layout_calculation_data_t &data,
                                     std::invocable<point_t> auto next_point) -> bool;

/**
 * @brief: Iterate the input locations and IDs
 *
 *  next_input = [](connection_id_t input_id, point_t position,
 *                  orientation_t orientation) -> bool
 *
 * The callable is called for each point or until it returns false.
 *
 * Returns the value of the last callable result.
 */
inline auto iter_input_location_and_id(
    const layout_calculation_data_t &data,
    std::invocable<connection_id_t, point_t, orientation_t> auto next_input) -> bool;

/**
 * @brief: Iterate the output locations and IDs
 *
 *  next_output = [](connection_id_t output_id, point_t position,
 *                   orientation_t orientation) -> bool
 *
 * The callable is called for each point or until it returns false.
 *
 * Returns the value of the last callable result.
 */
inline auto iter_output_location_and_id(
    const layout_calculation_data_t &data,
    std::invocable<connection_id_t, point_t, orientation_t> auto next_output) -> bool;

//
// Implementation
//

[[nodiscard]] auto iter_input_location(const layout_calculation_data_t &data)
    -> connectors_vector;

[[nodiscard]] auto iter_output_location(const layout_calculation_data_t &data)
    -> connectors_vector;

[[nodiscard]] auto iter_element_body_points(const layout_calculation_data_t &data)
    -> body_points_vector;

// inline auto iter_input_location_and_id(
//     const layout_calculation_data_t &data) -> bool {
//     return iter_input_location(
//         data, [&, input_id = connection_id_t {0}](point_t position,
//                                                   orientation_t orientation) mutable {
//             return std::invoke(next_input, input_id++, position, orientation);
//         });
// }
//
// inline auto iter_output_location_and_id(
//     const layout_calculation_data_t &data) -> bool {
//     return iter_output_location(
//         data, [&, output_id = connection_id_t {0}](point_t position,
//                                                    orientation_t orientation) mutable {
//             return std::invoke(next_output, output_id++, position, orientation);
//         });
// }

inline auto iter_input_location(const layout_calculation_data_t &data,
                                std::invocable<point_t, orientation_t> auto next_input)
    -> bool {
    return iter_input_location_base(
        data, [&](point_t position, orientation_t orientation) {
            return std::invoke(next_input,
                               transform(data.position, data.orientation, position),
                               transform(data.orientation, orientation));
        });
}

inline auto iter_output_location(const layout_calculation_data_t &data,
                                 std::invocable<point_t, orientation_t> auto next_output)
    -> bool {
    return iter_output_location_base(
        data, [&](point_t position, orientation_t orientation) {
            return std::invoke(next_output,
                               transform(data.position, data.orientation, position),
                               transform(data.orientation, orientation));
        });
}

inline auto iter_element_body_points(const layout_calculation_data_t &data,
                                     std::invocable<point_t> auto next_point) -> bool {
    return iter_element_body_points_base(data, [&](point_t position) {
        return std::invoke(next_point,
                           transform(data.position, data.orientation, position));
    });
}

inline auto iter_input_location_and_id(
    const layout_calculation_data_t &data,
    std::invocable<connection_id_t, point_t, orientation_t> auto next_input) -> bool {
    return iter_input_location(
        data, [&, input_id = connection_id_t {0}](point_t position,
                                                  orientation_t orientation) mutable {
            return std::invoke(next_input, input_id++, position, orientation);
        });
}

inline auto iter_output_location_and_id(
    const layout_calculation_data_t &data,
    std::invocable<connection_id_t, point_t, orientation_t> auto next_output) -> bool {
    return iter_output_location(
        data, [&, output_id = connection_id_t {0}](point_t position,
                                                   orientation_t orientation) mutable {
            return std::invoke(next_output, output_id++, position, orientation);
        });
}

}  // namespace logicsim

#endif
