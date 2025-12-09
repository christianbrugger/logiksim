#ifndef LOGICSIM_VOCABULARY_CONNECTION_COUNT_H
#define LOGICSIM_VOCABULARY_CONNECTION_COUNT_H

#include "core/algorithm/numeric.h"
#include "core/concept/explicitly_convertible.h"
#include "core/concept/integral.h"
#include "core/format/struct.h"
#include "core/type_trait/safe_difference_type.h"
#include "core/vocabulary/connection_id.h"

#include <compare>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Defines the number of inputs or outputs of an unspecified circuit element.
 *
 * Class invariants:
 *     * connection_count is in range [0, connection_id_t::max())
 */
struct connection_count_t {
   public:
    using value_type = std::make_unsigned_t<connection_id_t::value_type>;
    constexpr static inline auto value_min = value_type {0};
    constexpr static inline auto value_max =
        value_type {connection_id_t::max().value + 1};

    static_assert(sizeof(value_type) == sizeof(connection_id_t));

   private:
    value_type value {0};

   public:
    using difference_type = safe_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    [[nodiscard]] explicit constexpr connection_count_t() noexcept = default;
    [[nodiscard]] explicit constexpr connection_count_t(integral auto value_);

    [[nodiscard]] explicit constexpr operator std::size_t() const noexcept;
    [[nodiscard]] explicit constexpr operator std::ptrdiff_t() const noexcept;
    [[nodiscard]] explicit constexpr operator difference_type() const noexcept;

    // returns underlying representation
    [[nodiscard]] constexpr auto count() const noexcept -> value_type;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const connection_count_t &other) const
        -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const connection_count_t &other) const
        -> std::strong_ordering;
    [[nodiscard]] auto operator<=>(const connection_id_t &other) const
        -> std::strong_ordering;

    [[nodiscard]] static constexpr auto min() noexcept -> connection_count_t;
    [[nodiscard]] static constexpr auto max() noexcept -> connection_count_t;

    constexpr auto operator+=(const connection_count_t &other) -> connection_count_t &;
    constexpr auto operator-=(const connection_count_t &other) -> connection_count_t &;
    constexpr auto operator*=(const int &other) -> connection_count_t &;

    constexpr auto operator++() -> connection_count_t &;
    constexpr auto operator++(int) -> connection_count_t;

    constexpr auto operator--() -> connection_count_t &;
    constexpr auto operator--(int) -> connection_count_t;
};

static_assert(std::is_nothrow_default_constructible_v<connection_count_t>);
static_assert(std::is_trivially_copyable_v<connection_count_t>);
static_assert(std::is_trivially_copy_constructible_v<connection_count_t>);
static_assert(std::is_trivially_copy_assignable_v<connection_count_t>);
static_assert(
    explicitly_convertible_to<connection_count_t, connection_count_t::difference_type>);

[[nodiscard]] constexpr auto operator+(const connection_count_t &left,
                                       const connection_count_t &right)
    -> connection_count_t;
[[nodiscard]] constexpr auto operator-(const connection_count_t &left,
                                       const connection_count_t &right)
    -> connection_count_t;

[[nodiscard]] constexpr auto operator*(const connection_count_t &left,
                                       const int &right) -> connection_count_t;
[[nodiscard]] constexpr auto operator*(const int &left, const connection_count_t &right)
    -> connection_count_t;

//
// Implementation
//

constexpr connection_count_t::connection_count_t(integral auto value_)
    : value {gsl::narrow<value_type>(value_)} {
    if (value > value_max) [[unlikely]] {
        throw std::runtime_error {"connection count overflow"};
    }
};

constexpr connection_count_t::operator std::size_t() const noexcept {
    return std::size_t {value};
}

constexpr connection_count_t::operator std::ptrdiff_t() const noexcept {
    return std::ptrdiff_t {value};
}

constexpr connection_count_t::operator difference_type() const noexcept {
    return difference_type {value};
}

constexpr auto connection_count_t::count() const noexcept -> value_type {
    return value;
}

constexpr auto connection_count_t::operator<=>(const connection_count_t &other) const
    -> std::strong_ordering {
    return value <=> other.value;
}

constexpr auto connection_count_t::min() noexcept -> connection_count_t {
    constexpr static auto value = connection_count_t {value_min};
    return value;
};

constexpr auto connection_count_t::max() noexcept -> connection_count_t {
    constexpr static auto value = connection_count_t {value_max};
    return value;
};

constexpr auto connection_count_t::operator+=(const connection_count_t &other)
    -> connection_count_t & {
    value = checked_add(value, other.value);
    if (value > value_max) [[unlikely]] {
        throw std::runtime_error {"connection count overflow"};
    }
    return *this;
}

constexpr auto connection_count_t::operator-=(const connection_count_t &other)
    -> connection_count_t & {
    static_assert(std::unsigned_integral<value_type>);
    value = checked_sub(value, other.value);
    return *this;
}

constexpr auto connection_count_t::operator*=(const int &other) -> connection_count_t & {
    value = gsl::narrow<value_type>(checked_mul(int {value}, other));

    if (value > value_max) [[unlikely]] {
        throw std::runtime_error {"connection count overflow"};
    }

    return *this;
}

constexpr auto connection_count_t::operator++() -> connection_count_t & {
    value = checked_add(value, value_type {1});
    if (value > value_max) [[unlikely]] {
        throw std::runtime_error {"connection count overflow"};
    }
    return *this;
}

constexpr auto connection_count_t::operator++(int) -> connection_count_t {
    auto tmp = *this;
    operator++();
    return tmp;
}

constexpr auto connection_count_t::operator--() -> connection_count_t & {
    value = checked_sub(value, value_type {1});
    return *this;
}

constexpr auto connection_count_t::operator--(int) -> connection_count_t {
    auto tmp = *this;
    operator--();
    return tmp;
}

constexpr auto operator+(const connection_count_t &left,
                         const connection_count_t &right) -> connection_count_t {
    auto result = left;
    result += right;
    return result;
}

constexpr auto operator-(const connection_count_t &left,
                         const connection_count_t &right) -> connection_count_t {
    auto result = left;
    result -= right;
    return result;
}

constexpr auto operator*(const connection_count_t &left,
                         const int &right) -> connection_count_t {
    auto result = left;
    result *= right;
    return result;
}

constexpr auto operator*(const int &left,
                         const connection_count_t &right) -> connection_count_t {
    return operator*(right, left);
}

//
// Constants
//

static_assert(connection_count_t {} == connection_count_t {0});

}  // namespace logicsim

#endif
