#ifndef LOGICSIM_VOCABULARY_SELECTION_ID_H
#define LOGICSIM_VOCABULARY_SELECTION_ID_H

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

struct selection_id_t {
    using value_type = int64_t;
    value_type value;

    using difference_type = safe_difference_t<value_type>;
    static_assert(sizeof(difference_type) >= sizeof(value_type));

    [[nodiscard]] explicit constexpr selection_id_t() = default;
    [[nodiscard]] explicit constexpr selection_id_t(integral auto value);
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

    [[nodiscard]] auto operator==(const selection_id_t& other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const selection_id_t& other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept -> selection_id_t;

    constexpr auto operator++() -> selection_id_t&;
    constexpr auto operator++(int) -> selection_id_t;
};

static_assert(std::is_trivial_v<selection_id_t>);
static_assert(std::is_trivially_constructible_v<selection_id_t>);
static_assert(std::is_trivially_copyable_v<selection_id_t>);
static_assert(std::is_trivially_copy_assignable_v<selection_id_t>);
static_assert(explicitly_convertible_to<selection_id_t, selection_id_t::difference_type>);

//
// Implementation
//

constexpr selection_id_t::selection_id_t(integral auto value)
    : value {narrow_integral<value_type>(value)} {}

constexpr selection_id_t::operator std::size_t() const {
    if (value < value_type {0}) [[unlikely]] {
        throw std::runtime_error(
            "element id cannot be negative when converting to std::size_t");
    }
    return static_cast<std::size_t>(value);
}

constexpr selection_id_t::operator difference_type() const {
    return difference_type {value};
}

constexpr selection_id_t::operator bool() const noexcept {
    return value >= 0;
}

constexpr auto selection_id_t::max() noexcept -> selection_id_t {
    return selection_id_t {std::numeric_limits<value_type>::max()};
};

constexpr auto selection_id_t::operator++() -> selection_id_t& {
    if (value < value_type {0}) [[unlikely]] {
        throw std::runtime_error("element id cannot be negative when incrementing");
    }
    if (value >= max().value) [[unlikely]] {
        throw std::overflow_error("cannot increment, overflow");
    }
    ++value;
    return *this;
}

constexpr auto selection_id_t::operator++(int) -> selection_id_t {
    auto tmp = *this;
    operator++();
    return tmp;
}

//
// Constants
//
inline constexpr auto null_selection_id = selection_id_t {-1};

}  // namespace logicsim

//
// Hash function
//

template <>
struct ankerl::unordered_dense::hash<logicsim::selection_id_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::selection_id_t& obj) const noexcept
        -> uint64_t {
        return logicsim::wyhash(obj.value);
    }
};

#endif
