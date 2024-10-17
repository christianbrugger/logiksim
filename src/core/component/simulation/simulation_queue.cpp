#include "core/component/simulation/simulation_queue.h"

#include "core/algorithm/pop_while.h"
#include "core/component/simulation/simulation_event_group.h"

#include <stdexcept>

namespace logicsim {

namespace simulation {

//
// SimulationQueue
//

auto SimulationQueue::time() const noexcept -> time_t {
    return time_;
}

void SimulationQueue::set_time(time_t time) {
    if (time < time_) {
        throw std::runtime_error("Cannot set new time to the past.");
    }
    if (time > next_event_time()) {
        throw std::runtime_error("New time would be greater than next event.");
    }

    time_ = time;
}

auto SimulationQueue::next_event_time() const noexcept -> time_t {
    return events_.empty() ? time_t::max() : events_.top().time;
}

auto SimulationQueue::empty() const noexcept -> bool {
    return events_.empty();
}

void SimulationQueue::submit_event(simulation_event_t event) {
    if (event.time <= time_) {
        throw std::runtime_error("Event time needs to be in the future.");
    }

    events_.push(event);
}

auto SimulationQueue::pop_event_group() -> SimulationEventGroup {
    SimulationEventGroup group;
    pop_while(
        events_, [&group](const simulation_event_t &event) { group.push_back(event); },
        [&group](const simulation_event_t &event) {
            return group.empty() || (group.front().element_id == event.element_id &&
                                     group.front().time == event.time);
        });
    if (!group.empty()) {
        this->set_time(group.front().time);
    }
    return group;
}

}  // namespace simulation

}  // namespace logicsim
