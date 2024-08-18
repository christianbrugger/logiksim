#ifndef LOGICSIM_SIMULATION_PLAYER_H
#define LOGICSIM_SIMULATION_PLAYER_H

#include "component/simulation/simulation_event.h"

#include <vector>

namespace logicsim {

class Simulation;

/**
 * @brief: Runs a simulation with a given set of input events.
 *
 * Note inputs need to be unconnected.
 *
 * Note two events at the same time are scheduled at slightly different times,
 * as only one event can be submitted per ns.
 *
 * Throws if an event in the past is given.
 */
auto run_with_events(Simulation &simulation,
                     std::vector<simulation::simulation_event_t> &&events) -> void;

}  // namespace logicsim

#endif
