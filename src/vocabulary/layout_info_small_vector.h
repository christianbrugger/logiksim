#ifndef LOGICSIM_VOCABULARY_LAYOUT_INFO_SMALL_VECTOR_H
#define LOGICSIM_VOCABULARY_LAYOUT_INFO_SMALL_VECTOR_H

#include "container/static_vector.h"
#include "format/container.h"
#include "vocabulary/connector_info.h"
#include "vocabulary/point.h"

#include <folly/small_vector.h>

namespace logicsim {

/**
 * @brief:  Constexpr friendly vector able to hold statically defined connections
 *
 * Increase this as needed, when defining a new type with more connections.
 */
constexpr inline auto static_connectors_size = 8;
using static_connectors = static_vector<connector_info_t, static_connectors_size>;

/**
 * @brief:  Hold the input or output connections of a logic item.
 */
constexpr inline auto connectors_vector_size = 14;
using connectors_vector = folly::small_vector<connector_info_t, connectors_vector_size>;

/**
 * @brief:  Hold the body points connections of a logic item.
 */
constexpr inline auto body_points_vector_size = 28;
using body_points_vector = folly::small_vector<point_t, body_points_vector_size>;

}  // namespace logicsim

#endif
