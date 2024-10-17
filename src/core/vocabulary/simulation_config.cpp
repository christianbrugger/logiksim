#include "core/vocabulary/simulation_config.h"

#include <fmt/core.h>

namespace logicsim {

auto SimulationConfig::format() const -> std::string {
    return fmt::format(
        "SimulationConfig{{\n"
        "  simulation_time_rate = {},\n"
        "  use_wire_delay = {},\n"
        "}}",
        simulation_time_rate, use_wire_delay);
}

auto SimulationConfig::wire_delay_per_distance() const -> delay_t {
    return use_wire_delay ? default_wire_delay_per_distance() : delay_t {0ns};
}

namespace {
constexpr inline auto _default_wire_delay_per_distance = delay_t {1us};
// WARNING: safe-file behavior depend on this value staying the same
static_assert(_default_wire_delay_per_distance == delay_t {1us});
}  // namespace

auto default_wire_delay_per_distance() -> delay_t {
    return _default_wire_delay_per_distance;
}

}  // namespace logicsim
