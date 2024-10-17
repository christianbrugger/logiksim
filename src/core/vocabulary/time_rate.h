#ifndef LOGICSIM_VOCABULARY_TIME_RATE_H
#define LOGICSIM_VOCABULARY_TIME_RATE_H

#include "core/format/struct.h"
#include "core/vocabulary/delay.h"
#include "core/vocabulary/time_literal.h"

#include <compare>
#include <type_traits>

namespace logicsim {

/**
 * @brief: The rate at which the simulation time is advacing.
 *
 * The unit is simulation seconds / realtime seconds.
 */
struct time_rate_t {
    delay_t rate_per_second {};

    [[nodiscard]] explicit constexpr time_rate_t() = default;
    [[nodiscard]] explicit constexpr time_rate_t(
        std::chrono::duration<delay_t::rep, delay_t::period> time_rate) noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const time_rate_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const time_rate_t &other) const = default;
};

static_assert(std::is_trivially_copyable_v<time_rate_t>);
static_assert(std::is_trivially_copy_constructible_v<time_rate_t>);
static_assert(std::is_trivially_copy_assignable_v<time_rate_t>);

//
// Implementation
//

constexpr time_rate_t::time_rate_t(
    std::chrono::duration<delay_t::rep, delay_t::period> time_rate) noexcept
    : rate_per_second {delay_t {time_rate}} {}

}  // namespace logicsim

#endif
