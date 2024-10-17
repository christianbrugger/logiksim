#include "core/simulation_player.h"

#include "core/logging.h"
#include "core/simulation.h"

#include <algorithm>
#include <ranges>

namespace logicsim {

auto run_with_events(Simulation& simulation,
                     std::vector<simulation::simulation_event_t>&& events) -> void {
    using simulation::simulation_event_t;
    if (events.empty()) {
        return;
    }

    // sort twice, so our results are deterministic in terms of sorting
    std::ranges::sort(events);
    std::ranges::stable_sort(events, {}, &simulation_event_t::time);
    if (events.front().time < simulation.time()) [[unlikely]] {
        throw std::runtime_error("events in the past cannot be scheduled.");
    }

    for (const simulation_event_t& event_ : events) {
        const auto delay = event_.time - simulation.time();
        if (delay > delay_t::zero()) {
            simulation.run({.simulate_for = delay});
        }

        assert(simulation.time() >= event_.time);
        simulation.set_unconnected_input(input_t {event_.element_id, event_.input_id},
                                         event_.value);
    }
}

}  // namespace logicsim
