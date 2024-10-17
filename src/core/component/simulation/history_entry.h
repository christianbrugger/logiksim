#ifndef LOGICSIM_COMPONENT_SIMULATION_HISTORY_ENTRY_H
#define LOGICSIM_COMPONENT_SIMULATION_HISTORY_ENTRY_H

#include "core/format/struct.h"
#include "core/vocabulary/time.h"

#include <compare>
#include <type_traits>

namespace logicsim {

namespace simulation {

/**
 * @brief: History entry - first and last time of a specific logic value
 *
 * Class invariants:
 *     * first_time < last_time
 */
struct history_entry_t {
    // to construct with designated initializers
    struct New {
        time_t first_time;
        time_t last_time;
        bool value;
    };

    time_t first_time;
    time_t last_time;
    bool value;

    [[nodiscard]] explicit history_entry_t(New data);
    [[nodiscard]] explicit history_entry_t(time_t first_time, time_t last_time,
                                           bool value);

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const history_entry_t &) const -> bool = default;
    [[nodiscard]] auto operator<=>(const history_entry_t &) const = default;
};

static_assert(std::is_trivially_copyable_v<history_entry_t>);
static_assert(std::is_trivially_copy_assignable_v<history_entry_t>);

}  // namespace simulation

}  // namespace logicsim

#endif
