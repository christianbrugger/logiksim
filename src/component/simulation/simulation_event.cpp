#include "component/simulation/simulation_event.h"

#include <fmt/core.h>

namespace logicsim {

namespace simulation {

    
//
// Simulation Event
//

auto SimulationEvent::format() const -> std::string {
    return fmt::format("<SimulationEvent: at {} set Element_{}[{}] = {}>", time,
                       element_id, input_index, value);
}

auto SimulationEvent::operator==(const SimulationEvent &other) const -> bool {
    return this->element_id == other.element_id && this->time == other.time;
}

auto SimulationEvent::operator<(const SimulationEvent &other) const -> bool {
    if (this->time == other.time) {
        return this->element_id < other.element_id;
    }
    return this->time < other.time;
}

auto SimulationEvent::operator!=(const SimulationEvent &other) const -> bool {
    return !(this->operator==(other));
}

auto SimulationEvent::operator>(const SimulationEvent &other) const -> bool {
    return other.operator<(*this);
}

auto SimulationEvent::operator<=(const SimulationEvent &other) const -> bool {
    return !(this->operator>(other));
}

auto SimulationEvent::operator>=(const SimulationEvent &other) const -> bool {
    return !(this->operator<(other));
}

//
// Public Functions
//

auto make_event(SchematicOld::ConstInput input, time_t time, bool value)
    -> SimulationEvent {
    return {.time = time,
            .element_id = input.element_id(),
            .input_index = input.input_index(),
            .value = value};
}

}  // namespace simulation

}
