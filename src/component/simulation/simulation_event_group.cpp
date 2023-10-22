#include "component/simulation/simulation_event_group.h"

#include "algorithm/contains.h"

#include <stdexcept>

namespace logicsim {

namespace simulation {

auto SimulationEventGroup::empty() const -> bool {
    return group_.empty();
}

auto SimulationEventGroup::size() const -> std::size_t {
    return group_.size();
}

auto SimulationEventGroup::push_back(simulation_event_t event) -> void {
    if (!empty()) {
        if (event.element_id != front().element_id) [[unlikely]] {
            throw std::runtime_error("All events need to have the same element id.");
        }
        if (event.time != front().time) [[unlikely]] {
            throw std::runtime_error("All events need to have the same time.");
        }
        if (contains(group_, event.input_id,
                     [](const simulation_event_t &event) { return event.input_id; })) {
            throw std::runtime_error("All input ids need to be unique.");
        }
    }

    group_.push_back(event);
}

auto SimulationEventGroup::front() const -> const simulation_event_t & {
    return group_.front();
}

auto SimulationEventGroup::back() const -> const simulation_event_t & {
    return group_.back();
}

auto SimulationEventGroup::begin() const -> const_iterator {
    return group_.begin();
}

auto SimulationEventGroup::end() const -> const_iterator {
    return group_.end();
}

}  // namespace simulation

}  // namespace logicsim
