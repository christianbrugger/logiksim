#ifndef LOGICSIM_COMPONENT_SIMULATION_HISTORY_INDEX_H
#define LOGICSIM_COMPONENT_SIMULATION_HISTORY_INDEX_H

#include "format/struct.h"

#include <compare>
#include <limits>
#include <stdexcept>
#include <string>

namespace logicsim {

/**
 * @brief: Identifier to a history-entry in a history-buffer.
 */
struct history_index_t {
    using value_type = std::size_t;
    value_type value;

    using difference_type = std::ptrdiff_t;

    [[nodiscard]] explicit constexpr operator std::size_t() const;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const history_index_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const history_index_t &other) const = default;

    constexpr auto operator++() -> history_index_t &;
    constexpr auto operator++(int) -> history_index_t;
};

static_assert(std::is_trivial_v<history_index_t>);
static_assert(std::is_trivially_constructible_v<history_index_t>);
static_assert(std::is_trivially_copyable_v<history_index_t>);
static_assert(std::is_trivially_copy_assignable_v<history_index_t>);

//
// Implementation
//

constexpr history_index_t::operator std::size_t() const {
    return value;
}

constexpr auto history_index_t::operator++() -> history_index_t & {
    if (value == std::numeric_limits<value_type>::max()) [[unlikely]] {
        throw std::overflow_error("cannot increment, overflow");
    }
    ++value;
    return *this;
}

constexpr auto history_index_t::operator++(int) -> history_index_t {
    auto tmp = *this;
    operator++();
    return tmp;
}

}  // namespace logicsim

#endif
