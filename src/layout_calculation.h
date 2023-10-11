#ifndef LOGIKSIM_LAYOUT_CALCULATION_H
#define LOGIKSIM_LAYOUT_CALCULATION_H

#include "algorithm/range.h"
#include "exception.h"
#include "geometry/connection_count.h"
#include "logic_item/layout.h"
#include "logic_item/layout_display.h"
#include "logic_item/layout_display_ascii.h"
#include "logic_item/layout_display_number.h"
#include "logic_item/layout_standard_element.h"
#include "vocabulary.h"
#include "vocabulary/font_style.h"
#include "vocabulary/layout_calculation_data.h"

#include <array>
#include <vector>

struct BLPoint;

namespace logicsim {

// General
namespace defaults {
constexpr static inline auto line_selection_padding = grid_fine_t {0.3};
constexpr static inline auto logic_item_body_overdraw = grid_fine_t {0.4};
}  // namespace defaults

[[nodiscard]] auto is_input_output_count_valid(ElementType element_type,
                                               connection_count_t input_count,
                                               connection_count_t output_count) -> bool;
[[nodiscard]] auto is_orientation_valid(ElementType element_type,
                                        orientation_t orientation) -> bool;

[[nodiscard]] auto element_collision_rect(const layout_calculation_data_t &data)
    -> rect_t;
[[nodiscard]] auto element_selection_rect(const layout_calculation_data_t &data)
    -> rect_fine_t;
[[nodiscard]] auto element_selection_rect(ordered_line_t line) -> rect_fine_t;
[[nodiscard]] auto element_selection_rect_rounded(ordered_line_t line) -> rect_fine_t;
[[nodiscard]] auto element_bounding_rect(const layout_calculation_data_t &data) -> rect_t;

[[nodiscard]] auto is_representable(layout_calculation_data_t data) -> bool;
[[nodiscard]] auto orientations_compatible(orientation_t a, orientation_t b) -> bool;

[[nodiscard]] auto transform(point_t element_position, orientation_t orientation,
                             point_t offset) -> point_t;
[[nodiscard]] auto transform(point_t element_position, orientation_t orientation,
                             point_fine_t offset) -> point_fine_t;

[[nodiscard]] auto transform(orientation_t element_orientation, orientation_t connector)
    -> orientation_t;

auto connector_point(point_t position, orientation_t orientation, grid_fine_t offset)
    -> point_fine_t;
auto connector_point(BLPoint position, orientation_t orientation, double offset)
    -> BLPoint;

constexpr auto get_fixed_width(ElementType element_type) -> grid_t {
    const auto info = get_layout_info(element_type);
    if (info.variable_width) [[unlikely]] {
        throw std::runtime_error("element has variable width");
    }
    return info.fixed_width;
}

constexpr auto get_fixed_height(ElementType element_type) -> grid_t {
    const auto info = get_layout_info(element_type);
    if (info.variable_height) [[unlikely]] {
        throw std::runtime_error("element has variable height");
    }
    return info.fixed_height;
}

constexpr auto default_input_count(ElementType element_type) -> connection_count_t {
    return get_layout_info(element_type).input_count_default;
}

constexpr auto default_output_count(ElementType element_type) -> connection_count_t {
    return get_layout_info(element_type).output_count_default;
}

constexpr auto get_direction_type(ElementType element_type) -> DirectionType {
    return get_layout_info(element_type).direction_type;
}

// next_input = [](point_t position, orientation_t orientation) -> bool
auto iter_input_location(const layout_calculation_data_t &data,
                         std::invocable<point_t, orientation_t> auto next_input) -> bool {
    return iter_input_location_base(
        data, [&](point_t position, orientation_t orientation) {
            return std::invoke(next_input,
                               transform(data.position, data.orientation, position),
                               transform(data.orientation, orientation));
        });
}

// next_output = [](point_t position, orientation_t orientation) -> bool
auto iter_output_location(const layout_calculation_data_t &data,
                          std::invocable<point_t, orientation_t> auto next_output)
    -> bool {
    return iter_output_location_base(
        data, [&](point_t position, orientation_t orientation) {
            return std::invoke(next_output,
                               transform(data.position, data.orientation, position),
                               transform(data.orientation, orientation));
        });
}

// next_point = [](point_t position) -> bool
auto iter_element_body_points(const layout_calculation_data_t &data,
                              std::invocable<point_t> auto next_point) -> bool {
    return iter_element_body_points_base(data, [&](point_t position) {
        return std::invoke(next_point,
                           transform(data.position, data.orientation, position));
    });
}

// next_input(connection_id_t input_id, point_t position,
//            orientation_t orientation) -> bool;
auto iter_input_location_and_id(
    const layout_calculation_data_t &data,
    std::invocable<connection_id_t, point_t, orientation_t> auto next_input) -> bool {
    return iter_input_location(
        data, [&, input_id = connection_id_t {0}](point_t position,
                                                  orientation_t orientation) mutable {
            return std::invoke(next_input, input_id++, position, orientation);
        });
}

// next_output(connection_id_t output_id, point_t position,
//             orientation_t orientation) -> bool;
auto iter_output_location_and_id(
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
