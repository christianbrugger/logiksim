#ifndef LOGICSIM_VOCABULARY_WIRE_ID_H
#define LOGICSIM_VOCABULARY_WIRE_ID_H

#include "algorithm/narrow_integral.h"
#include "concept/explicitly_convertible.h"
#include "concept/integral.h"
#include "format/struct.h"
#include "type_trait/safe_difference_type.h"
#include "wyhash.h"

#include <ankerl/unordered_dense.h>
#include <gsl/gsl>

#include <compare>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Identifier to a wire in the layout.
 */
struct wire_id_t {
    using value_type = int32_t;
    value_type value;

    using difference_type = safe_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    [[nodiscard]] explicit constexpr wire_id_t() = default;
    [[nodiscard]] explicit constexpr wire_id_t(integral auto value_);

    /**
     * @brief: The conversion to std::size_t
     *
     * Note When indexing arrays use .at(id.value) instead, due to performance reasons.
     *
     * Throws exception for negative / invalid ids.
     */
    [[nodiscard]] explicit constexpr operator std::size_t() const;

    [[nodiscard]] explicit constexpr operator difference_type() const;

    /**
     * @brief: The bool cast tests if this ID is valid.
     */
    [[nodiscard]] explicit constexpr operator bool() const noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const wire_id_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const wire_id_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept -> wire_id_t;

    constexpr auto operator++() -> wire_id_t &;
    constexpr auto operator++(int) -> wire_id_t;
};

static_assert(std::is_trivial_v<wire_id_t>);
static_assert(std::is_trivially_constructible_v<wire_id_t>);
static_assert(std::is_trivially_copyable_v<wire_id_t>);
static_assert(std::is_trivially_copy_assignable_v<wire_id_t>);
static_assert(explicitly_convertible_to<wire_id_t, wire_id_t::difference_type>);

[[nodiscard]] constexpr auto is_inserted(wire_id_t wire_id) -> bool;
[[nodiscard]] constexpr auto is_temporary(wire_id_t wire_id) -> bool;
[[nodiscard]] constexpr auto is_colliding(wire_id_t wire_id) -> bool;

//
// Implementation
//

constexpr wire_id_t::wire_id_t(integral auto value_)
    : value {narrow_integral<value_type>(value_)} {}

constexpr wire_id_t::operator std::size_t() const {
    if (value < value_type {0}) [[unlikely]] {
        throw std::runtime_error(
            "wire id cannot be negative when converting to std::size_t");
    }
    return static_cast<std::size_t>(value);
}

constexpr wire_id_t::operator difference_type() const {
    return difference_type {value};
}

constexpr wire_id_t::operator bool() const noexcept {
    return value >= 0;
}

constexpr auto wire_id_t::max() noexcept -> wire_id_t {
    return wire_id_t {std::numeric_limits<value_type>::max()};
};

constexpr auto wire_id_t::operator++() -> wire_id_t & {
    if (value < value_type {0}) [[unlikely]] {
        throw std::runtime_error("wire id cannot be negative when incrementing");
    }
    if (value >= max().value) [[unlikely]] {
        throw std::overflow_error("cannot increment, overflow");
    }
    ++value;
    return *this;
}

constexpr auto wire_id_t::operator++(int) -> wire_id_t {
    auto tmp = *this;
    operator++();
    return tmp;
}

//
// Constants
//

constexpr inline static auto null_wire_id = wire_id_t {-1};
constexpr inline static auto temporary_wire_id = wire_id_t {0};
constexpr inline static auto colliding_wire_id = wire_id_t {1};
constexpr inline static auto first_inserted_wire_id = wire_id_t {2};

//
// Free Methods
//

constexpr auto is_inserted(wire_id_t wire_id) -> bool {
    return wire_id >= first_inserted_wire_id;
}

constexpr auto is_temporary(wire_id_t wire_id) -> bool {
    return wire_id == temporary_wire_id;
}

constexpr auto is_colliding(wire_id_t wire_id) -> bool {
    return wire_id == colliding_wire_id;
}

}  // namespace logicsim

//
// Hash function
//

template <>
struct ankerl::unordered_dense::hash<logicsim::wire_id_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::wire_id_t &obj) const noexcept
        -> uint64_t {
        return logicsim::wyhash(obj.value);
    }
};

#endif
