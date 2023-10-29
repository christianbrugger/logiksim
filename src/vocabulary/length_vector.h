#ifndef LOGICSIM_VOCABULARY_LENGTH_VECTOR_H
#define LOGICSIM_VOCABULARY_LENGTH_VECTOR_H

#include "vocabulary/length.h"

#include <folly/small_vector.h>

namespace logicsim {

//
// Small Vectors
//

using length_vector_policy = folly::small_vector_policy::policy_size_type<uint32_t>;
constexpr inline auto length_vector_size = 9;

using length_vector_t =
    folly::small_vector<length_t, length_vector_size, length_vector_policy>;

static_assert(sizeof(length_vector_t) == 40);

}  // namespace logicsim

#endif
