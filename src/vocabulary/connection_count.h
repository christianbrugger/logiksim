#ifndef LOGICSIM_VOCABULARY_CONNECTION_COUNT_H
#define LOGICSIM_VOCABULARY_CONNECTION_COUNT_H

#include "difference_type.h"
#include "format/struct.h"
#include "vocabulary/connection_id.h"

#include <gsl/gsl>

#include <compare>
#include <stdexcept>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Defines the number of inputs or outputs of an unspecified circuit element.
 */
// TODO: Use this from Layout
// using connection_size_t = std::make_unsigned<connection_id_t::value_type>::type;
// static_assert(sizeof(connection_size_t) == sizeof(connection_id_t));
struct connection_count_t {
    using value_type = std::size_t;
    value_type value;

    using difference_type = safe_difference_t<value_type>;
    // TODO enable
    // static_assert(sizeof(difference_type) > sizeof(value_type));

    // TODO add constructor
    // TODO check input count is not bigger than MAX
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const connection_count_t &other) const
        -> bool = default;
    [[nodiscard]] auto operator<=>(const connection_count_t &other) const = default;
    [[nodiscard]] auto operator<=>(const connection_id_t &other) const
        -> std::strong_ordering;

    [[nodiscard]] static constexpr auto min() noexcept -> connection_count_t;
    [[nodiscard]] static constexpr auto max() noexcept -> connection_count_t;

    [[nodiscard]] constexpr auto operator+(connection_count_t other) const
        -> connection_count_t;
    [[nodiscard]] constexpr auto operator-(connection_count_t other) const
        -> connection_count_t;
    [[nodiscard]] constexpr auto operator*(int other) const -> connection_count_t;

    constexpr auto operator++() -> connection_count_t &;
    constexpr auto operator++(int) -> connection_count_t;
};

static_assert(std::is_trivial<connection_id_t>::value);

//
// Implementation
//

inline constexpr auto connection_count_t::min() noexcept -> connection_count_t {
    return connection_count_t {0};
}

// TODO check who is using this !!!
constexpr auto connection_count_t::max() noexcept -> connection_count_t {
    return connection_count_t {connection_id_t::max().value};
};

constexpr auto connection_count_t::operator+(connection_count_t other) const
    -> connection_count_t {
    if (std::numeric_limits<value_type>::max() - other.value < value) {
        throw std::runtime_error("overflow when adding to connection counts");
    }
    return connection_count_t {value + other.value};
}

constexpr auto connection_count_t::operator-(connection_count_t other) const
    -> connection_count_t {
    if (other.value > value) {
        throw std::runtime_error("overflow when substracting two connection counts");
    }
    return connection_count_t {value - other.value};
}

constexpr auto connection_count_t::operator*(int other) const -> connection_count_t {
    if (other == 0) {
        return connection_count_t {0};
    }
    if (other < 0 || max().value / other >= value) {
        throw std::runtime_error("multiplication overflow for connection_count_t");
    }
    return connection_count_t {static_cast<std::size_t>(other) * value};
}

constexpr auto connection_count_t::operator++() -> connection_count_t & {
    return *this = *this + connection_count_t {1};
}

constexpr auto connection_count_t::operator++(int) -> connection_count_t {
    auto tmp = *this;
    operator++();
    return tmp;
}

}  // namespace logicsim

#endif
