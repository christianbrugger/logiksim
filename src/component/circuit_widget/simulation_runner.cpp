#include "component/circuit_widget/simulation_runner.h"

#include "algorithm/checked_deref.h"
#include "interactive_simulation.h"

namespace logicsim {

namespace circuit_widget {

auto run_simulation(InteractiveSimulation& interactive_simulation,
                    realtime_timeout_t simulation_timeout) -> bool {
    const auto was_finished = interactive_simulation.is_finished();

    interactive_simulation.run(simulation_timeout);

    return !was_finished;
}

auto run_simulation(InteractiveSimulation* interactive_simulation,
                    realtime_timeout_t simulation_timeout) -> bool {
    return run_simulation(checked_deref(interactive_simulation), simulation_timeout);
}

}  // namespace circuit_widget

}  // namespace logicsim
