#ifndef LOGIKSIM_SIMULATION_TYPE
#define LOGIKSIM_SIMULATION_TYPE

#include "vocabulary.h"

namespace logicsim {

struct SimulationSettings {
    time_rate_t simulation_time_rate {10us};
    bool use_wire_delay {true};

    [[nodiscard]] auto wire_delay_per_distance() const -> delay_t;
};

}  // namespace logicsim

#endif
