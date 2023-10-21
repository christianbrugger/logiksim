#include "component/simulation/event_group.h"

#include "algorithm/has_duplicates_quadratic.h"

#include <stdexcept>

namespace logicsim {

namespace simulation {

//
// EventGroup
//

void validate(const event_group_t &events) {
    if (events.empty()) {
        return;
    }

    const auto &head {events.front()};
    const auto tail = std::ranges::subrange(events.begin() + 1, events.end());

    if (head.element_id == null_element) {
        throw std::runtime_error("Event element cannot be null.");
    }

    if (!tail.empty()) {
        if (!std::ranges::all_of(tail, [time = head.time](const SimulationEvent &event) {
                return event.time == time;
            })) {
            throw std::runtime_error(
                "All events in the group need to have the same time.");
        }

        if (!std::ranges::all_of(
                tail, [element_id = head.element_id](const SimulationEvent &event) {
                    return event.element_id == element_id;
                })) {
            throw std::runtime_error(
                "All events in the group need to have the same time.");
        }

        const auto to_index = [](const SimulationEvent &event) {
            return event.input_index;
        };
        if (has_duplicates_quadratic(events, to_index)) {
            throw std::runtime_error(
                "Cannot have two events for the same input at the same time.");
        }
    }
}

}  // namespace simulation

}  // namespace logicsim
