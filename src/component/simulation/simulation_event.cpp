#include "component/simulation/simulation_event.h"

#include <fmt/core.h>

namespace logicsim {

namespace simulation {

//
// Simulation Event
//

auto simulation_event_t::format() const -> std::string {
    return fmt::format("<SimulationEvent: at {} set Element_{}[{}] = {}>", time,
                       element_id, input_id, value);
}

// auto simulation_event_t::operator==(const simulation_event_t &other) const -> bool {
//     return this->element_id == other.element_id && this->time == other.time;
// }
//
// auto simulation_event_t::operator<(const simulation_event_t &other) const -> bool {
//     if (this->time == other.time) {
//         return this->element_id < other.element_id;
//     }
//     return this->time < other.time;
// }
//
// auto simulation_event_t::operator!=(const simulation_event_t &other) const -> bool {
//     return !(this->operator==(other));
// }
//
// auto simulation_event_t::operator>(const simulation_event_t &other) const -> bool {
//     return other.operator<(*this);
// }
//
// auto simulation_event_t::operator<=(const simulation_event_t &other) const -> bool {
//     return !(this->operator>(other));
// }
//
// auto simulation_event_t::operator>=(const simulation_event_t &other) const -> bool {
//     return !(this->operator<(other));
// }

//
// Public Functions
//

auto greater_time_element_id::operator()(const simulation_event_t &left,
                                         const simulation_event_t &right) const -> bool {
    if (left.time == right.time) {
        return left.element_id > right.element_id;
    }
    return left.time > right.time;
}

auto make_event(SchematicOld::ConstInput input, time_t time, bool value)
    -> simulation_event_t {
    return {.time = time,
            .element_id = input.element_id(),
            .input_id = input.input_index(),
            .value = value};
}

}  // namespace simulation

}  // namespace logicsim
