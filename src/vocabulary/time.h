#ifndef LOGICSIM_VOCABULARY_TIME_H
#define LOGICSIM_VOCABULARY_TIME_H

#include "format/struct.h"
#include "vocabulary/time_literal.h"

#include <chrono>
#include <compare>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Specifies the current simulation time point
 */
struct time_t {
    using value_type = std::chrono::duration<int64_t, std::nano>;
    using rep = value_type::rep;
    value_type value;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const time_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const time_t &other) const = default;

    [[nodiscard]] static constexpr auto zero() noexcept -> time_t {
        return time_t {value_type::zero()};
    };

    [[nodiscard]] static constexpr auto epsilon() noexcept -> time_t {
        return time_t {++value_type::zero()};
    };

    [[nodiscard]] static constexpr auto min() noexcept -> time_t {
        return time_t {value_type::min()};
    };

    [[nodiscard]] static constexpr auto max() noexcept -> time_t {
        return time_t {value_type::max()};
    };
};

static_assert(std::is_trivial<time_t>::value);

}  // namespace logicsim

#endif
