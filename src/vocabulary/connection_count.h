#ifndef LOGICSIM_VOCABULARY_CONNECTION_COUNT_H
#define LOGICSIM_VOCABULARY_CONNECTION_COUNT_H

#include "concept/integral.h"
#include "format/struct.h"
#include "safe_numeric.h"
#include "type_trait/safe_difference_type.h"
#include "vocabulary/connection_id.h"

#include <compare>
#include <stdexcept>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Defines the number of inputs or outputs of an unspecified circuit element.
 */
struct connection_count_t {
    using value_type_rep = std::make_unsigned_t<connection_id_t::value_type>;
    using value_type =
        ls_safe_range<value_type_rep, 0, std::size_t {connection_id_t::max()} + 1>;
    static_assert(sizeof(value_type) == sizeof(connection_id_t));

   private:
    value_type value;

   public:
    using difference_type = safe_difference_t<value_type_rep>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    [[nodiscard]] explicit constexpr connection_count_t() noexcept;
    [[nodiscard]] explicit constexpr connection_count_t(integral auto value);
    template <class Stored, Stored Min, Stored Max, class P, class E>
    [[nodiscard]] explicit constexpr connection_count_t(
        boost::safe_numerics::safe_base<Stored, Min, Max, P, E> value);

    [[nodiscard]] explicit constexpr operator std::size_t() const noexcept;
    // returns safe_numerics value
    [[nodiscard]] constexpr auto safe_value() const noexcept -> value_type;
    // returns underlying representation
    [[nodiscard]] constexpr auto count() const noexcept -> value_type_rep;

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

static_assert(std::is_trivially_copyable_v<connection_count_t>);
static_assert(std::is_trivially_copy_constructible_v<connection_count_t>);
static_assert(std::is_trivially_copy_assignable_v<connection_count_t>);

[[nodiscard]] constexpr auto operator+(const connection_count_t &left,
                                       const connection_count_t &right)
    -> connection_count_t;
[[nodiscard]] constexpr auto operator-(const connection_count_t &left,
                                       const connection_count_t &right)
    -> connection_count_t;

[[nodiscard]] constexpr auto operator*(const connection_count_t &left, const int &right)
    -> connection_count_t;
[[nodiscard]] constexpr auto operator*(const int &left, const connection_count_t &right)
    -> connection_count_t;

[[nodiscard]] auto first_connection_id(connection_count_t count) -> connection_id_t;
[[nodiscard]] auto last_connection_id(connection_count_t count) -> connection_id_t;

//
// Implementation
//

constexpr connection_count_t::connection_count_t() noexcept : value {0} {};

constexpr connection_count_t::connection_count_t(integral auto value_)
    : value {value_} {};

template <class Stored, Stored Min, Stored Max, class P, class E>
constexpr connection_count_t::connection_count_t(
    boost::safe_numerics::safe_base<Stored, Min, Max, P, E> value_)
    : value {value_} {}

constexpr connection_count_t::operator std::size_t() const noexcept {
    return std::size_t {value};
}

[[nodiscard]] constexpr auto connection_count_t::safe_value() const noexcept
    -> value_type {
    return value;
}

constexpr auto connection_count_t::count() const noexcept -> value_type_rep {
    return value_type_rep {value};
}

constexpr auto connection_count_t::operator<=>(const connection_count_t &other) const
    -> std::strong_ordering {
    return value_type_rep {value} <=> value_type_rep {other.value};
}

constexpr auto connection_count_t::min() noexcept -> connection_count_t {
    constexpr auto value = connection_count_t {std::numeric_limits<value_type>::min()};
    return value;
};

constexpr auto connection_count_t::max() noexcept -> connection_count_t {
    constexpr auto value = connection_count_t {std::numeric_limits<value_type>::max()};
    return value;
};

constexpr auto connection_count_t::operator+=(const connection_count_t &other)
    -> connection_count_t & {
    value += other.value;
    return *this;
}

constexpr auto connection_count_t::operator-=(const connection_count_t &other)
    -> connection_count_t & {
    value -= other.value;
    return *this;
}

constexpr auto connection_count_t::operator*=(const int &other) -> connection_count_t & {
    value *= ls_safe<int>(other);
    return *this;
}

constexpr auto connection_count_t::operator++() -> connection_count_t & {
    ++value;
    return *this;
}

constexpr auto connection_count_t::operator++(int) -> connection_count_t {
    auto tmp = *this;
    operator++();
    return tmp;
}

constexpr auto connection_count_t::operator--() -> connection_count_t & {
    --value;
    return *this;
}

constexpr auto connection_count_t::operator--(int) -> connection_count_t {
    auto tmp = *this;
    operator--();
    return tmp;
}

constexpr auto operator+(const connection_count_t &left, const connection_count_t &right)
    -> connection_count_t {
    auto result = left;
    result += right;
    return result;
}

constexpr auto operator-(const connection_count_t &left, const connection_count_t &right)
    -> connection_count_t {
    auto result = left;
    result -= right;
    return result;
}

constexpr auto operator*(const connection_count_t &left, const int &right)
    -> connection_count_t {
    auto result = left;
    result *= right;
    return result;
}

constexpr auto operator*(const int &left, const connection_count_t &right)
    -> connection_count_t {
    return operator*(right, left);
}

}  // namespace logicsim

#endif
