#ifndef LOGICSIM_VOCABULARY_OPTIONAL_LOGIC_VALUES_H
#define LOGICSIM_VOCABULARY_OPTIONAL_LOGIC_VALUES_H

#include "core/format/container.h"
#include "core/vocabulary/optional_logic_value.h"

#include <folly/small_vector.h>

#include <cstdint>

namespace logicsim {

constexpr inline auto optional_logic_values_size = 20;

/**
 * @brief: Static vector of optional logic values.
 */
using optional_logic_values_t =
    folly::small_vector<OptionalLogicValue, optional_logic_values_size>;

static_assert(sizeof(optional_logic_values_t) == 48);

}  // namespace logicsim

#endif
