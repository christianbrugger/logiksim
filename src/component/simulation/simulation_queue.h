#ifndef LOGICSIM_COMPONENT_SIMULATION_SIMULATION_QUEUE_H
#define LOGICSIM_COMPONENT_SIMULATION_SIMULATION_QUEUE_H

#include "component/simulation/event_group.h"
#include "component/simulation/simulation_event.h"
#include "vocabulary/time.h"

#include <queue>

namespace logicsim {

namespace simulation {

class SimulationQueue {
    using queue_t = std::priority_queue<SimulationEvent, std::vector<SimulationEvent>,
                                        std::greater<>>;

   public:
    [[nodiscard]] auto time() const noexcept -> time_t;
    [[nodiscard]] auto next_event_time() const noexcept -> time_t;
    [[nodiscard]] auto empty() const noexcept -> bool;

    void set_time(time_t time);
    void submit_event(SimulationEvent event);

    /**
     * brief: Collects all events for the next timepoint and advances the simulation time.
     *
     * Event group contains all input events at the same time and for the same element id.
     */
    auto pop_event_group() -> event_group_t;

   private:
    time_t time_ {0us};
    queue_t events_;
};
}  // namespace simulation

}  // namespace logicsim

#endif
