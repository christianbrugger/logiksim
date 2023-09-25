#ifndef LOGICSIM_VOCABULARY_DELAY_H
#define LOGICSIM_VOCABULARY_DELAY_H

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
    value_type value {};

    [[nodiscard]] constexpr explicit delay_t() noexcept = default;

    [[nodiscard]] constexpr explicit delay_t(
        std::chrono::duration<int64_t, std::nano> delay)
        : value {delay} {
        if (value != delay) {
            throw std::runtime_error("delay cannot be represented.");
        }
    };

    [[nodiscard]] static constexpr auto epsilon() noexcept -> delay_t {
        return delay_t {++value_type::zero()};
    };

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const delay_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const delay_t &other) const = default;

    [[nodiscard]] static constexpr auto min() noexcept -> delay_t {
        return delay_t {value_type::min()};
    };

    [[nodiscard]] static constexpr auto max() noexcept -> delay_t {
        return delay_t {value_type::max()};
    };
};

}  // namespace logicsim

#endif
