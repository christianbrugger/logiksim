#ifndef LOGIKSIM_VOCABULARY_ELEMENT_ID_H
#define LOGIKSIM_VOCABULARY_ELEMENT_ID_H

#include "algorithm/narrow_integral.h"
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
 * @brief: Identifier to an element in the circuit, schematic or layout.
 */
struct element_id_t {
    using value_type = int32_t;
    value_type value;

    using difference_type = safe_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    [[nodiscard]] explicit constexpr element_id_t() = default;
    [[nodiscard]] explicit constexpr element_id_t(integral auto value);

    /**
     * @brief: The conversion to std::size_t is used for indexing into vectors.
     *
     * An exception is thrown, if the id is not valid.
     */
    [[nodiscard]] explicit constexpr operator std::size_t() const noexcept;
    /**
     * @brief: The bool cast tests if this ID is valid.
     */
    [[nodiscard]] explicit constexpr operator bool() const noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const element_id_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const element_id_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept -> element_id_t;

    constexpr auto operator++() noexcept -> element_id_t &;
    constexpr auto operator++(int) noexcept -> element_id_t;
};

static_assert(std::is_trivial_v<element_id_t>);
static_assert(std::is_trivially_constructible_v<element_id_t>);
static_assert(std::is_trivially_copyable_v<element_id_t>);
static_assert(std::is_trivially_copy_assignable_v<element_id_t>);

//
// Implementation
//

constexpr element_id_t::element_id_t(integral auto value)
    : value {narrow_integral<value_type>(value)} {}

constexpr element_id_t::operator std::size_t() const noexcept {
    // throws error for negative / invalid ids
    return gsl::narrow<std::size_t>(value);
}

constexpr element_id_t::operator bool() const noexcept {
    return value >= 0;
}

constexpr auto element_id_t::max() noexcept -> element_id_t {
    return element_id_t {std::numeric_limits<value_type>::max()};
};

constexpr auto element_id_t::operator++() noexcept -> element_id_t & {
    ++value;
    return *this;
}

constexpr auto element_id_t::operator++(int) noexcept -> element_id_t {
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
