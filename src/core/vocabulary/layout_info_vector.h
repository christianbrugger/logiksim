#ifndef LOGICSIM_VOCABULARY_LAYOUT_INFO_VECTOR_H
#define LOGICSIM_VOCABULARY_LAYOUT_INFO_VECTOR_H

#include "container/static_vector.h"
#include "format/container.h"
#include "vocabulary/connector_info.h"
#include "vocabulary/point.h"

#include <folly/small_vector.h>

namespace logicsim {

/**
 * @brief:  Constexpr friendly vector able to hold statically defined inputs
 *
 * Increase this as needed, when defining a new type with more inputs.
 */
constexpr inline auto static_inputs_size = 8;
using static_inputs_t = static_vector<simple_input_info_t, static_inputs_size>;

/**
 * @brief:  Constexpr friendly vector able to hold statically defined outputs
 *
 * Increase this as needed, when defining a new type with more outputs.
 */
constexpr inline auto static_outputs_size = 8;
using static_outputs_t = static_vector<simple_output_info_t, static_outputs_size>;

/**
 * @brief:  Hold the input connections of a logic item.
 */
constexpr inline auto inputs_vector_size = 14;
using inputs_vector = folly::small_vector<simple_input_info_t, inputs_vector_size>;

/**
 * @brief:  Hold the output connections of a logic item.
 */
constexpr inline auto outputs_vector_size = 14;
using outputs_vector = folly::small_vector<simple_output_info_t, outputs_vector_size>;

/**
 * @brief:  Hold the body points connections of a logic item.
 */
constexpr inline auto body_points_vector_size = 28;
using body_points_vector = folly::small_vector<point_t, body_points_vector_size>;

}  // namespace logicsim

#endif
