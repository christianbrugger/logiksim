#ifndef LOGIKSIM_VOCABULARY_ELEMENT_ID_H
#define LOGIKSIM_VOCABULARY_ELEMENT_ID_H

#include "algorithm/narrow_integral.h"
#include "concept/explicitly_convertible.h"
#include "concept/integral.h"
#include "format/struct.h"
#include "type_trait/safe_difference_type.h"
#include "wyhash.h"

#include <ankerl/unordered_dense.h>

#include <compare>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Identifier to an element in the schematic.
 */
struct element_id_t {
    using value_type = int32_t;
    value_type value;

    using difference_type = safe_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    [[nodiscard]] explicit constexpr element_id_t() = default;
    [[nodiscard]] explicit constexpr element_id_t(integral auto value_);

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

    [[nodiscard]] auto operator==(const element_id_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const element_id_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept -> element_id_t;

    constexpr auto operator++() -> element_id_t &;
    constexpr auto operator++(int) -> element_id_t;
};

static_assert(std::is_trivial_v<element_id_t>);
static_assert(std::is_trivially_constructible_v<element_id_t>);
static_assert(std::is_trivially_copyable_v<element_id_t>);
static_assert(std::is_trivially_copy_assignable_v<element_id_t>);
static_assert(explicitly_convertible_to<element_id_t, element_id_t::difference_type>);

//
// Implementation
//

constexpr element_id_t::element_id_t(integral auto value_)
    : value {narrow_integral<value_type>(value_)} {}

constexpr element_id_t::operator std::size_t() const {
    if (value < value_type {0}) [[unlikely]] {
        throw std::runtime_error(
            "element id cannot be negative when converting to std::size_t");
    }
    return static_cast<std::size_t>(value);
}

constexpr element_id_t::operator difference_type() const {
    return difference_type {value};
}

constexpr element_id_t::operator bool() const noexcept {
    return value >= 0;
}

constexpr auto element_id_t::max() noexcept -> element_id_t {
    return element_id_t {std::numeric_limits<value_type>::max()};
};

constexpr auto element_id_t::operator++() -> element_id_t & {
    if (value < value_type {0}) [[unlikely]] {
        throw std::runtime_error("element id cannot be negative when incrementing");
    }
    if (value >= max().value) [[unlikely]] {
        throw std::overflow_error("cannot increment, overflow");
    }
    ++value;
    return *this;
}

constexpr auto element_id_t::operator++(int) -> element_id_t {
    auto tmp = *this;
    operator++();
    return tmp;
}

//
// Constants
//

constexpr inline static auto null_element = element_id_t {-1};

}  // namespace logicsim

//
// Hash function
//

template <>
struct ankerl::unordered_dense::hash<logicsim::element_id_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::element_id_t &obj) const noexcept
        -> uint64_t {
        return logicsim::wyhash(obj.value);
    }
};

#endif
