#ifndef LOGICSIM_CORE_ELEMENT_DECORATION_LAYOUT_DESCRIPTION_H
#define LOGICSIM_CORE_ELEMENT_DECORATION_LAYOUT_DESCRIPTION_H

#include "vocabulary/decoration_type.h"
#include "vocabulary/layout_info_vector.h"

namespace logicsim {

struct size_2d_t;
struct decoration_layout_data_t;

namespace layout_info {

[[nodiscard]] auto is_decoration_size_valid(DecorationType decoration_type,
                                            size_2d_t size) -> bool;

/**
 * @brief: Returns vector of body points, type point_t.
 *
 * Note this is the base version, not considering element position or orientation.
 */
[[nodiscard]] auto decoration_body_points(const decoration_layout_data_t& data)
    -> body_points_vector;

}  // namespace layout_info

}  // namespace logicsim

#endif
