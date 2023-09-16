#include "simulation_type.h"

namespace logicsim {

auto SimulationSettings::wire_delay_per_distance() const -> delay_t {
    return use_wire_delay ? defaults::wire_delay_per_distance : delay_t {0ns};
}

}  // namespace logicsim