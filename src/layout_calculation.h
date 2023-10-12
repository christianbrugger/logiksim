#ifndef LOGIKSIM_LAYOUT_CALCULATION_H
#define LOGIKSIM_LAYOUT_CALCULATION_H

#include "geometry/connection_count.h"
#include "geometry/layout1.h"  // TODO rename & remove
#include "geometry/layout2.h"  // TODO rename & remove
#include "logic_item/layout.h"
#include "vocabulary/connection_id.h"
#include "vocabulary/element_type.h"
#include "vocabulary/grid.h"
#include "vocabulary/grid_fine.h"
#include "vocabulary/layout_calculation_data.h"
#include "vocabulary/point.h"

#include <concepts>

struct BLPoint;

namespace logicsim {

struct rect_t;
struct rect_fine_t;
struct ordered_line_t;

// TODO put in c++ file
namespace defaults {
constexpr static inline auto line_selection_padding = grid_fine_t {0.3};
constexpr static inline auto logic_item_body_overdraw = grid_fine_t {0.4};
}  // namespace defaults

[[nodiscard]] auto is_input_output_count_valid(ElementType element_type,
                                               connection_count_t input_count,
                                               connection_count_t output_count) -> bool;
[[nodiscard]] auto is_orientation_valid(ElementType element_type,
                                        orientation_t orientation) -> bool;

[[nodiscard]] auto is_representable(layout_calculation_data_t data) -> bool;

//
//
//

[[nodiscard]] constexpr auto element_input_count_min(ElementType element_type)
    -> connection_count_t;
[[nodiscard]] constexpr auto element_input_count_max(ElementType element_type)
    -> connection_count_t;
[[nodiscard]] constexpr auto element_input_count_default(ElementType element_type)
    -> connection_count_t;

[[nodiscard]] constexpr auto element_output_count_min(ElementType element_type)
    -> connection_count_t;
[[nodiscard]] constexpr auto element_output_count_max(ElementType element_type)
    -> connection_count_t;
[[nodiscard]] constexpr auto element_output_count_default(ElementType element_type)
    -> connection_count_t;

[[nodiscard]] constexpr auto element_direction_type(ElementType element_type)
    -> DirectionType;

[[nodiscard]] constexpr auto element_fixed_width(ElementType element_type) -> grid_t;
[[nodiscard]] constexpr auto element_fixed_height(ElementType element_type) -> grid_t;
[[nodiscard]] constexpr auto element_fixed_size(ElementType element_type) -> point_t;

[[nodiscard]] auto element_size(const layout_calculation_data_t &data) -> point_t;

[[nodiscard]] auto element_body_rect(const layout_calculation_data_t &data)
    -> rect_fine_t;

[[nodiscard]] auto element_collision_rect(const layout_calculation_data_t &data)
    -> rect_t;

// TODO collision rect for lines

[[nodiscard]] auto element_bounding_rect(const layout_calculation_data_t &data) -> rect_t;

// TODO bounding rect for lines

[[nodiscard]] auto element_selection_rect(const layout_calculation_data_t &data)
    -> rect_fine_t;

[[nodiscard]] auto element_selection_rect(ordered_line_t line) -> rect_fine_t;

[[nodiscard]] auto element_shadow_rect(const layout_calculation_data_t &data)
    -> rect_fine_t;

[[nodiscard]] auto element_shadow_rect(ordered_line_t line) -> rect_fine_t;

//
// Iterators
//

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
//
//

//
// Implementation
//

constexpr auto element_input_count_min(ElementType element_type) -> connection_count_t {
    return get_layout_info(element_type).input_count_min;
}

constexpr auto element_input_count_max(ElementType element_type) -> connection_count_t {
    return get_layout_info(element_type).input_count_max;
}

constexpr auto element_input_count_default(ElementType element_type)
    -> connection_count_t {
    return get_layout_info(element_type).input_count_default;
}

constexpr auto element_output_count_min(ElementType element_type) -> connection_count_t {
    return get_layout_info(element_type).output_count_min;
}

constexpr auto element_output_count_max(ElementType element_type) -> connection_count_t {
    return get_layout_info(element_type).output_count_max;
}

constexpr auto element_output_count_default(ElementType element_type)
    -> connection_count_t {
    return get_layout_info(element_type).output_count_default;
}

constexpr auto element_direction_type(ElementType element_type) -> DirectionType {
    return get_layout_info(element_type).direction_type;
}

constexpr auto element_fixed_width(ElementType element_type) -> grid_t {
    const auto info = get_layout_info(element_type);
    if (info.variable_width) [[unlikely]] {
        throw std::runtime_error("element has variable width");
    }
    return info.fixed_width;
}

constexpr auto element_fixed_height(ElementType element_type) -> grid_t {
    const auto info = get_layout_info(element_type);
    if (info.variable_height) [[unlikely]] {
        throw std::runtime_error("element has variable height");
    }
    return info.fixed_height;
}

constexpr auto element_fixed_size(ElementType element_type) -> point_t {
    return point_t {element_fixed_width(element_type),
                    element_fixed_height(element_type)};
}

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
