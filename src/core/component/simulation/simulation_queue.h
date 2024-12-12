#ifndef LOGICSIM_COMPONENT_SIMULATION_SIMULATION_QUEUE_H
#define LOGICSIM_COMPONENT_SIMULATION_SIMULATION_QUEUE_H

#include "core/component/simulation/simulation_event.h"
#include "core/format/struct.h"
#include "core/vocabulary/time.h"

#include <queue>
#include <string>

namespace logicsim {

namespace simulation {

class SimulationEventGroup;

namespace simulation_queue {

using event_priority_queue =
    std::priority_queue<simulation_event_t, std::vector<simulation_event_t>,
                        greater_time_element_id>;

class queue_t : public event_priority_queue {
   public:
    using event_priority_queue::priority_queue;

    // TODO define equality
    // [[nodiscard]] auto operator==(const queue_t &) const -> bool = default;

    [[nodiscard]] auto data() const -> const container_type&;
};

}  // namespace simulation_queue

/**
 * @brief: Stores Simulation Events in order
 *
 * Class invariants:
 *     * event.time >= time_, for all events in the queue
 *     * time_ is never decreasing
 */
class SimulationQueue {
    // using queue_t =
    //     std::priority_queue<simulation_event_t, std::vector<simulation_event_t>,
    //                         greater_time_element_id>;
    using queue_t = simulation_queue::queue_t;

   public:
    // TODO define equality
    // [[nodiscard]] auto operator==(const SimulationQueue&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

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
