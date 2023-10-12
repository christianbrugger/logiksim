#ifndef LOGIKSIM_LAYOUT_INFO_H
#define LOGIKSIM_LAYOUT_INFO_H

#include "vocabulary/direction_type.h"
#include "vocabulary/element_type.h"
#include "vocabulary/orientation.h"

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

[[nodiscard]] auto is_input_output_count_valid(ElementType element_type,
                                               connection_count_t input_count,
                                               connection_count_t output_count) -> bool;
[[nodiscard]] auto is_orientation_valid(ElementType element_type,
                                        orientation_t orientation) -> bool;

[[nodiscard]] auto is_representable(layout_calculation_data_t data) -> bool;

//
// Connection Count
//

[[nodiscard]] auto element_input_count_min(ElementType element_type)
    -> connection_count_t;
[[nodiscard]] auto element_input_count_max(ElementType element_type)
    -> connection_count_t;
[[nodiscard]] auto element_input_count_default(ElementType element_type)
    -> connection_count_t;

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
// Element Size
//

[[nodiscard]] auto element_fixed_width(ElementType element_type) -> grid_t;
[[nodiscard]] auto element_fixed_height(ElementType element_type) -> grid_t;
[[nodiscard]] auto element_fixed_size(ElementType element_type) -> point_t;

[[nodiscard]] auto element_width(const layout_calculation_data_t &data) -> grid_t;
[[nodiscard]] auto element_height(const layout_calculation_data_t &data) -> grid_t;
[[nodiscard]] auto element_size(const layout_calculation_data_t &data) -> point_t;

[[nodiscard]] auto element_body_draw_rect_untransformed(const layout_calculation_data_t &data)
    -> rect_fine_t;
[[nodiscard]] auto element_body_draw_rect(const layout_calculation_data_t &data)
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

}  // namespace logicsim

#endif
