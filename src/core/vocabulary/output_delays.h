#ifndef LOGICSIM_VOCABULARY_OUTPUT_DELAYS_H
#define LOGICSIM_VOCABULARY_OUTPUT_DELAYS_H

#include "core/vocabulary/delay.h"

#include <folly/small_vector.h>

namespace logicsim {

/**
 * @brief: We store output delays in schematic for each element.
 *         So count needs to be small.
 */
constexpr inline auto output_delays_count = 3;

/**
 * @brief: List of output delays.
 */
using output_delays_t = folly::small_vector<delay_t, output_delays_count>;

static_assert(sizeof(delay_t) == 8);
static_assert(sizeof(output_delays_t) == 32);

}  // namespace logicsim

#endif
