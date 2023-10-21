#include "component/simulation/event_group.h"

#include "algorithm/contains.h"
#include "algorithm/has_duplicates_quadratic.h"  // TODO remove

#include <stdexcept>

namespace logicsim {

namespace simulation {

auto EventGroup::empty() const -> bool {
    return group_.empty();
}

auto EventGroup::size() const -> std::size_t {
    return group_.size();
}

auto EventGroup::push_back(simulation_event_t event) -> void {
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

auto EventGroup::front() const -> const simulation_event_t & {
    return group_.front();
}

auto EventGroup::back() const -> const simulation_event_t & {
    return group_.back();
}

auto EventGroup::begin() const -> const_iterator {
    return group_.begin();
}

auto EventGroup::end() const -> const_iterator {
    return group_.end();
}

}  // namespace simulation

}  // namespace logicsim
