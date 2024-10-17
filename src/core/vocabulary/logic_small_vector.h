#ifndef LOGICSIM_VOCABULARY_LOGIC_SMALL_VECTOR_H
#define LOGICSIM_VOCABULARY_LOGIC_SMALL_VECTOR_H

#include "core/format/container.h"

#include <folly/small_vector.h>

#include <cstdint>

namespace logicsim {

using logic_small_vector_policy = folly::small_vector_policy::policy_size_type<uint32_t>;

/**
 * @brief: Static vector of logic values.
 */
using logic_small_vector_t = folly::small_vector<bool, 20, logic_small_vector_policy>;

static_assert(sizeof(logic_small_vector_t) == 24);

}  // namespace logicsim

#endif
