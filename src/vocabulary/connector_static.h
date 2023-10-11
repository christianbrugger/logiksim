#ifndef LOGICSIM_VOCABULARY_CONNECTOR_STATIC_H
#define LOGICSIM_VOCABULARY_CONNECTOR_STATIC_H

#include "container/static_vector.h"
#include "vocabulary/connector_info.h"

namespace logicsim {

// increase this as needed to hold all fixed connections
constexpr inline auto connector_vector_t_max_size = 8;

using static_connectors = static_vector<connector_info_t, connector_vector_t_max_size>;

}  // namespace logicsim

#endif
