#ifndef LOGICSIM_COMPONENT_SIMULATION_EVENT_GROUP_H
#define LOGICSIM_COMPONENT_SIMULATION_EVENT_GROUP_H

#include "component/simulation/simulation_event.h"

#include <folly/small_vector.h>

namespace logicsim {

namespace simulation {

// TODO make proper class that hold invariant

/**
 * @brief: groups of events for the same element and time from different inputs
 */
using event_group_t = folly::small_vector<SimulationEvent, 4>;

// TODO make this part of class invariant
void validate(const event_group_t &events);

}  // namespace simulation

}  // namespace logicsim

#endif
