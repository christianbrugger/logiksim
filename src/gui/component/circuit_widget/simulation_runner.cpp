#include "gui/component/circuit_widget/simulation_runner.h"

#include "core/algorithm/checked_deref.h"
#include "core/interactive_simulation.h"

namespace logicsim {

namespace circuit_widget {

auto run_simulation(InteractiveSimulation& interactive_simulation,
                    realtime_timeout_t simulation_timeout) -> bool {
    const auto was_finished = interactive_simulation.is_finished();

    interactive_simulation.run(simulation_timeout);

    return !was_finished;
}

}  // namespace circuit_widget

}  // namespace logicsim
