#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_SIMULATION_RUNNER_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_SIMULATION_RUNNER_H

#include "core/vocabulary/realtime_timeout.h"

namespace logicsim {

class InteractiveSimulation;

namespace circuit_widget {

/**
 * @brief: Runs the simulation and returns true if an render update is required.
 */
[[nodiscard]] auto run_simulation(InteractiveSimulation& interactive_simulation,
                                  realtime_timeout_t simulation_timeout) -> bool;

}  // namespace circuit_widget

}  // namespace logicsim

#endif
