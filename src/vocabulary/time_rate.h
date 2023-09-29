#ifndef LOGICSIM_VOCABULARY_TIME_RATE_H
#define LOGICSIM_VOCABULARY_TIME_RATE_H

#include "format/struct.h"
#include "vocabulary/time.h"
#include "vocabulary/time_literal.h"

#include <compare>

namespace logicsim {

/**
 * @brief: The rate at which the simulation time is advacing.
 *
 * The unit is simulation seconds / realtime seconds.
 */
struct time_rate_t {
    time_t rate_per_second;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const time_rate_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const time_rate_t &other) const = default;
};

static_assert(std::is_aggregate_v<time_rate_t>);

}  // namespace logicsim

#endif
