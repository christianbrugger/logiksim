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

}  // namespace simulation

}  // namespace logicsim
