#ifndef LOGICSIM_VOCABULARY_SIMULATION_CONFIG_H
#define LOGICSIM_VOCABULARY_SIMULATION_CONFIG_H

#include "core/format/struct.h"
#include "core/vocabulary/delay.h"
#include "core/vocabulary/time_literal.h"
#include "core/vocabulary/time_rate.h"

#include <string>

namespace logicsim {

struct SimulationConfig {
    time_rate_t simulation_time_rate {10us};
    bool use_wire_delay {true};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const SimulationConfig &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const SimulationConfig &other) const = default;

    [[nodiscard]] auto wire_delay_per_distance() const -> delay_t;
};

[[nodiscard]] auto default_wire_delay_per_distance() -> delay_t;

}  // namespace logicsim

#endif
