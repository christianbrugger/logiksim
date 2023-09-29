#ifndef LOGICSIM_VOCABULARY_DELAY_H
#define LOGICSIM_VOCABULARY_DELAY_H

#include "concept/integral.h"
#include "format/struct.h"
#include "vocabulary/time_literal.h"

#include <chrono>
#include <compare>
#include <stdexcept>

namespace logicsim {

/**
 * @brief: Specifies a delay in simulation time for wires or events
 */
struct delay_t {
    using value_type = std::chrono::duration<int32_t, std::nano>;
    using rep = value_type::rep;
    value_type value;

    [[nodiscard]] explicit constexpr delay_t() noexcept = default;
    [[nodiscard]] explicit constexpr delay_t(
        std::chrono::duration<int64_t, std::nano> delay);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const delay_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const delay_t &other) const = default;

    [[nodiscard]] static constexpr auto epsilon() noexcept -> delay_t;
    [[nodiscard]] static constexpr auto min() noexcept -> delay_t;
    [[nodiscard]] static constexpr auto max() noexcept -> delay_t;
};

static_assert(std::is_trivial_v<delay_t>);

//
// Implementation
//

constexpr delay_t::delay_t(std::chrono::duration<int64_t, std::nano> delay)
    : value {delay} {
    if (value != delay) {
        throw std::runtime_error("delay cannot be represented.");
    }
};

constexpr auto delay_t::epsilon() noexcept -> delay_t {
    return delay_t {++value_type::zero()};
};

constexpr auto delay_t::min() noexcept -> delay_t {
    return delay_t {value_type::min()};
};

constexpr auto delay_t::max() noexcept -> delay_t {
    return delay_t {value_type::max()};
};

}  // namespace logicsim

#endif
