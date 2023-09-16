#ifndef LOGIKSIM_SIMULATION_TYPE
#define LOGIKSIM_SIMULATION_TYPE

#include "vocabulary.h"

namespace logicsim {

namespace defaults {
constexpr static inline auto button_delay = delay_t::epsilon();
constexpr static inline auto logic_item_delay = delay_t {3us};
constexpr static inline auto wire_delay_per_distance = delay_t {1us};
}  // namespace defaults

struct SimulationSettings {
    time_rate_t simulation_time_rate {10us};
    bool use_wire_delay {true};

    [[nodiscard]] auto wire_delay_per_distance() const -> delay_t;
};

}  // namespace logicsim

#endif
