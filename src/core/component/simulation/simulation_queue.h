#ifndef LOGICSIM_COMPONENT_SIMULATION_SIMULATION_QUEUE_H
#define LOGICSIM_COMPONENT_SIMULATION_SIMULATION_QUEUE_H

#include "component/simulation/simulation_event.h"
#include "vocabulary/time.h"

#include <queue>

namespace logicsim {

namespace simulation {

class SimulationEventGroup;

/**
 * @brief: Stores Simulation Events in order
 *
 * Class invariants:
 *     * event.time >= time_, for all events in the queue
 *     * time_ is never decreasing
 */
class SimulationQueue {
    using queue_t =
        std::priority_queue<simulation_event_t, std::vector<simulation_event_t>,
                            greater_time_element_id>;

   public:
    [[nodiscard]] auto time() const noexcept -> time_t;
    [[nodiscard]] auto next_event_time() const noexcept -> time_t;
    [[nodiscard]] auto empty() const noexcept -> bool;

    /**
     * @brief: Set the simulation time.
     *
     * Throws, if given time is in the past or events are scheduled in between.
     */
    void set_time(time_t time);

    /**
     * @brief: Submit a new event to the queue.
     *
     * Pre-condition: It is not allowed to submit an events for the same time and input
     *                as an event that is already part of the queue.
     *
     * Note that this pre-condition is not checked by te queue, as it is expensive.
     * Breaking this eventually lead to a future exception when pop_event_group is called.
     */
    void submit_event(simulation_event_t event);

    /**
     * brief: Returns the next events and advances the simulation time.
     *
     * Events for the same time and element_id are grouped and returned at the same time.
     *
     * If queue is empty, returns an empty group. The time is not increased.
     */
    auto pop_event_group() -> SimulationEventGroup;

   private:
    time_t time_ {0us};
    queue_t events_;
};
}  // namespace simulation

}  // namespace logicsim

#endif
