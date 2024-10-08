#ifndef LOGICSIM_CORE_ELEMENT_DECORATION_LAYOUT_DESCRIPTION_H
#define LOGICSIM_CORE_ELEMENT_DECORATION_LAYOUT_DESCRIPTION_H

#include "vocabulary/layout_info_vector.h"

namespace logicsim {

struct offset_t;
struct DecorationDefinition;
struct decoration_layout_data_t;

[[nodiscard]] auto is_decoration_definition_valid(const DecorationDefinition& data)
    -> bool;

/**
 * @brief: Return width of decoration.
 */
[[nodiscard]] auto decoration_width(const DecorationDefinition& data) -> offset_t;

/**
 * @brief: Return height of the decoration.
 */
[[nodiscard]] auto decoration_height(const DecorationDefinition& data) -> offset_t;

/**
 * @brief: Returns vector of body points, type point_t.
 *
 * Note this is the base version, not considering element position or orientation.
 */
[[nodiscard]] auto decoration_body_points(const decoration_layout_data_t& data)
    -> body_points_vector;

}  // namespace logicsim

#endif
