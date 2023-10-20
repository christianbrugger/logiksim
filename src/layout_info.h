#ifndef LOGIKSIM_LAYOUT_INFO_H
#define LOGIKSIM_LAYOUT_INFO_H

#include "iterator_adaptor/enumerate.h"
#include "iterator_adaptor/transform_view.h"
#include "vocabulary/direction_type.h"
#include "vocabulary/element_type.h"
#include "vocabulary/layout_info_vector.h"
#include "vocabulary/orientation.h"

#include <optional>

namespace logicsim {

struct grid_t;
struct grid_fine_t;
struct point_t;
struct rect_t;
struct rect_fine_t;
struct ordered_line_t;
struct connection_count_t;
struct layout_calculation_data_t;

//
// Constants
//

[[nodiscard]] auto line_selection_padding() -> grid_fine_t;
[[nodiscard]] auto logic_item_body_overdraw() -> grid_fine_t;
[[nodiscard]] auto button_body_overdraw() -> grid_fine_t;

//
// Validation
//

/**
 * brief: Checks if the input and output count is valid.
 */
[[nodiscard]] auto is_input_output_count_valid(ElementType element_type,
                                               connection_count_t input_count,
                                               connection_count_t output_count) -> bool;

/**
 * brief: Checks if an element can be fully placed on the grid.
 */
[[nodiscard]] auto is_orientation_valid(ElementType element_type,
                                        orientation_t orientation) -> bool;

/**
 * brief: Checks if an element can be fully placed on the grid.
 */
[[nodiscard]] auto is_representable(layout_calculation_data_t data) -> bool;

/**
 * brief: Checks if an element is valid.
 *
 * Not this checks all of the above methods:
 *   * is_input_output_count_valid
 *   * is_orientation_valid
 *   * is_representable
 */
[[nodiscard]] auto is_valid(const layout_calculation_data_t &data) -> bool;

//
// Connection Count
//

/**
 * @brief: Input count information of an element.
 */
[[nodiscard]] auto element_input_count_min(ElementType element_type)
    -> connection_count_t;
[[nodiscard]] auto element_input_count_max(ElementType element_type)
    -> connection_count_t;
[[nodiscard]] auto element_input_count_default(ElementType element_type)
    -> connection_count_t;

/**
 * @brief: Output count information of an element.
 */
[[nodiscard]] auto element_output_count_min(ElementType element_type)
    -> connection_count_t;
[[nodiscard]] auto element_output_count_max(ElementType element_type)
    -> connection_count_t;
[[nodiscard]] auto element_output_count_default(ElementType element_type)
    -> connection_count_t;

//
// Direction Type
//

[[nodiscard]] auto element_direction_type(ElementType element_type) -> DirectionType;

//
// Connection Ids
//

[[nodiscard]] auto element_enable_input_id(ElementType element_type)
    -> std::optional<connection_id_t>;

//
// Element Size
//

/**
 * @brief: returns the fixed demensions of an element.
 *
 * Throws an exception if the element type has variable width.
 */
[[nodiscard]] auto element_fixed_width(ElementType element_type) -> grid_t;
[[nodiscard]] auto element_fixed_height(ElementType element_type) -> grid_t;
[[nodiscard]] auto element_fixed_size(ElementType element_type) -> point_t;

/**
 * @brief: calculates the demensions of an element.
 */
[[nodiscard]] auto element_width(const layout_calculation_data_t &data) -> grid_t;
[[nodiscard]] auto element_height(const layout_calculation_data_t &data) -> grid_t;
[[nodiscard]] auto element_size(const layout_calculation_data_t &data) -> point_t;

/**
 * @brief: The untransformed drawing rect of the logic item, if it has one.
 */
[[nodiscard]] auto element_body_draw_rect_untransformed(
    const layout_calculation_data_t &data) -> rect_fine_t;
/**
 * @brief: The transformed drawing rect of the logic item, if it has one.
 */
[[nodiscard]] auto element_body_draw_rect(const layout_calculation_data_t &data)
    -> rect_fine_t;

/**
 * @brief: A rect covering all grid points that the element occupies.
 */
[[nodiscard]] auto element_bounding_rect(const layout_calculation_data_t &data) -> rect_t;
[[nodiscard]] auto element_bounding_rect(ordered_line_t line) -> rect_t;

/**
 * @brief: The selection rect of an element.
 */
[[nodiscard]] auto element_selection_rect(const layout_calculation_data_t &data)
    -> rect_fine_t;
[[nodiscard]] auto element_selection_rect(ordered_line_t line) -> rect_fine_t;

/**
 * @brief: The shadow rect of an element.
 */
[[nodiscard]] auto element_shadow_rect(const layout_calculation_data_t &data)
    -> rect_fine_t;
[[nodiscard]] auto element_shadow_rect(ordered_line_t line) -> rect_fine_t;

//
// Input & Outputs & Body Points
//

/**
 * @brief: Returns vector of simple_input_info_t
 */
[[nodiscard]] auto input_locations(const layout_calculation_data_t &data)
    -> inputs_vector;

/**
 * @brief: Returns vector of simple_output_info_t
 */
[[nodiscard]] auto output_locations(const layout_calculation_data_t &data)
    -> outputs_vector;

/**
 * @brief: Returns vector of body points, type point_t.
 */
[[nodiscard]] auto element_body_points(const layout_calculation_data_t &data)
    -> body_points_vector;

/**
 * @brief: Returns range of extended_input_info_t
 */
[[nodiscard]] inline auto input_locations_and_id(const layout_calculation_data_t &data);

/**
 * @brief: Returns range of extended_output_info_t
 */
[[nodiscard]] inline auto output_locations_and_id(const layout_calculation_data_t &data);

//
// Implementation
//

[[nodiscard]] inline auto input_locations_and_id(const layout_calculation_data_t &data) {
    return transform_view(
        enumerate<connection_id_t>(input_locations(data)),
        [](auto pair) { return extend_input_info(pair.first, pair.second); });
}

[[nodiscard]] inline auto output_locations_and_id(const layout_calculation_data_t &data) {
    return transform_view(
        enumerate<connection_id_t>(output_locations(data)),
        [](auto pair) { return extend_output_info(pair.first, pair.second); });
}

}  // namespace logicsim

#endif
