#ifndef LOGICSIM_VOCABULARY_CONNECTION_IDS_H
#define LOGICSIM_VOCABULARY_CONNECTION_IDS_H

#include "vocabulary/connection_count.h"
#include "vocabulary/connection_id.h"

#include <folly/small_vector.h>

namespace logicsim {

//
// Small Vectors
//
using connection_ids_policy = folly::small_vector_policy::policy_size_type<uint32_t>;
constexpr inline auto connection_ids_size = 10;

using connection_ids_t =
    folly::small_vector<connection_id_t, connection_ids_size, connection_ids_policy>;
static_assert(sizeof(connection_ids_t) == 24);
static_assert(connection_ids_t::max_size() >= std::size_t {connection_count_t::max()});

}  // namespace logicsim

#endif
