#ifndef LOGICSIM_VOCABULARY_INTERNAL_CONNECTIONS_H
#define LOGICSIM_VOCABULARY_INTERNAL_CONNECTIONS_H

#include "core/vocabulary/connection_count.h"
#include "core/vocabulary/internal_connection.h"

#include <folly/small_vector.h>

namespace logicsim {

using internal_connection_policy = folly::small_vector_policy::policy_size_type<uint32_t>;
constexpr inline auto internal_connection_size = 7;

using internal_connections_t =
    folly::small_vector<internal_connection_t, internal_connection_size,
                        internal_connection_policy>;

static_assert(sizeof(internal_connections_t) == 32);

static_assert(internal_connections_t::max_size() >=
              std::size_t {connection_count_t::max()});

}  // namespace logicsim

#endif
