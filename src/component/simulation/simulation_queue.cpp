#include "component/simulation/simulation_queue.h"

#include "algorithm/pop_while.h"

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

void SimulationQueue::submit_event(SimulationEvent event) {
    if (event.time <= time_) {
        throw std::runtime_error("Event time needs to be in the future.");
    }

    events_.push(event);
}

auto SimulationQueue::pop_event_group() -> event_group_t {
    event_group_t group;
    pop_while(
        events_, [&group](const SimulationEvent &event) { group.push_back(event); },
        [&group](const SimulationEvent &event) {
            return group.empty() || group.front() == event;
        });
    if (!group.empty()) {
        this->set_time(group.front().time);
    }
    return group;
}

}  // namespace simulation

}  // namespace logicsim
