#ifndef LOGICSIM_COMPONENT_SIMULATION_HISTORY_BUFFER_H
#define LOGICSIM_COMPONENT_SIMULATION_HISTORY_BUFFER_H

#include "container/circular_buffer.h"
#include "vocabulary/time.h"

namespace logicsim {

namespace simulation {

using history_buffer_t = circular_buffer<time_t, 2, uint32_t>;

static_assert(sizeof(history_buffer_t) == 28);

}  // namespace simulation

}  // namespace logicsim

#endif
