#ifndef LOGICSIM_CORE_ELEMENT_DECORATION_LAYOUT_DESCRIPTION_H
#define LOGICSIM_CORE_ELEMENT_DECORATION_LAYOUT_DESCRIPTION_H

#include "vocabulary/layout_info_vector.h"

namespace logicsim {

struct decoration_layout_data_t;
/**
 * @brief: Returns vector of body points, type point_t.
 *
 * Note this is the base version, not considering element position or orientation.
 */
[[nodiscard]] auto decoration_body_points(const decoration_layout_data_t& data)
    -> body_points_vector;

}  // namespace logicsim

#endif
