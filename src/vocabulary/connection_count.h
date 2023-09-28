#ifndef LOGICSIM_VOCABULARY_CONNECTION_COUNT_H
#define LOGICSIM_VOCABULARY_CONNECTION_COUNT_H

#include "difference_type.h"
#include "format/struct.h"
#include "safe_numeric.h"
#include "vocabulary/connection_id.h"

#include <compare>
#include <stdexcept>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Defines the number of inputs or outputs of an unspecified circuit element.
 */
// TODO: introduce parameter ranges 0 - 128
struct connection_count_t {
    using value_type_rep = std::make_unsigned_t<connection_id_t::value_type>;
    using value_type = ls_safe<value_type_rep>;
    static_assert(sizeof(value_type) == sizeof(connection_id_t));

   private:
    value_type value;

   public:
    using difference_type = safe_difference_t<value_type_rep>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    [[nodiscard]] explicit constexpr connection_count_t() noexcept;
    // [[nodiscard]] explicit constexpr connection_count_t(value_type value_) noexcept;
    [[nodiscard]] explicit constexpr connection_count_t(integral auto value);

    template <class Stored, Stored Min, Stored Max, class P, class E>
    [[nodiscard]] explicit constexpr connection_count_t(
        boost::safe_numerics::safe_base<Stored, Min, Max, P, E> value);

    [[nodiscard]] explicit constexpr operator std::size_t() const noexcept;
    [[nodiscard]] constexpr auto safe_value() const noexcept -> value_type;
    [[nodiscard]] constexpr auto count() const noexcept -> value_type_rep;

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

    constexpr auto operator--() -> connection_count_t &;
    constexpr auto operator--(int) -> connection_count_t;
};

static_assert(std::is_trivially_copyable_v<connection_count_t>);
static_assert(std::is_trivially_copy_assignable_v<connection_count_t>);

[[nodiscard]] auto first_connection_id(connection_count_t count) -> connection_id_t;
[[nodiscard]] auto last_connection_id(connection_count_t count) -> connection_id_t;

//
// Implementation
//

constexpr connection_count_t::connection_count_t() noexcept : value {0} {};

// TODO remove narrow
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

constexpr auto connection_count_t::min() noexcept -> connection_count_t {
    return connection_count_t {0};
};

// TODO check who is using this !!!
// TODO add one
constexpr auto connection_count_t::max() noexcept -> connection_count_t {
    constexpr value_type value = value_type {connection_id_t::max().value};
    return connection_count_t {value};
};

// constexpr auto connection_count_t::max() noexcept -> connection_count_t {
//     constexpr auto value = value_type {connection_id_t::max().value} + 1;
//     return connection_count_t {value};
// };

constexpr auto connection_count_t::operator+(connection_count_t other) const
    -> connection_count_t {
    return connection_count_t {value + other.value};
}

constexpr auto connection_count_t::operator-(connection_count_t other) const
    -> connection_count_t {
    return connection_count_t {value - other.value};
}

constexpr auto connection_count_t::operator*(int other) const -> connection_count_t {
    return connection_count_t {other * value};
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
    ++value;
    return *this;
}

constexpr auto connection_count_t::operator--(int) -> connection_count_t {
    auto tmp = *this;
    operator--();
    return tmp;
}

}  // namespace logicsim

#endif
