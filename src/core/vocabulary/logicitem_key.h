#ifndef LOGICSIM_CORE_VOCABULARY_LOGICITEM_KEY_H
#define LOGICSIM_CORE_VOCABULARY_LOGICITEM_KEY_H

#include "core/algorithm/narrow_integral.h"
#include "core/concept/explicitly_convertible.h"
#include "core/concept/integral.h"
#include "core/format/struct.h"
#include "core/type_trait/safe_difference_type.h"
#include "core/wyhash.h"

#include <ankerl/unordered_dense.h>
#include <gsl/gsl>

#include <compare>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Unique identifier to an logicitem in the layout.
 */
struct logicitem_key_t {
    using value_type = int64_t;
    value_type value {-1};

    using difference_type = safe_difference_t<value_type>;

    [[nodiscard]] explicit constexpr logicitem_key_t() = default;
    [[nodiscard]] explicit constexpr logicitem_key_t(integral auto value_);

    [[nodiscard]] explicit constexpr operator std::size_t() const;
    [[nodiscard]] explicit constexpr operator difference_type() const;

    /**
     * @brief: The bool cast tests if this Key is valid.
     */
    [[nodiscard]] explicit constexpr operator bool() const noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const logicitem_key_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const logicitem_key_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept -> logicitem_key_t;

    constexpr auto operator++() -> logicitem_key_t &;
    constexpr auto operator++(int) -> logicitem_key_t;
};

static_assert(std::is_trivially_copyable_v<logicitem_key_t>);
static_assert(std::is_trivially_copy_assignable_v<logicitem_key_t>);
static_assert(
    explicitly_convertible_to<logicitem_key_t, logicitem_key_t::difference_type>);

//
// Implementation
//

constexpr logicitem_key_t::logicitem_key_t(integral auto value_)
    : value {narrow_integral<value_type>(value_)} {}

constexpr logicitem_key_t::operator std::size_t() const {
    if (value < value_type {0}) [[unlikely]] {
        throw std::runtime_error(
            "logicitem_id cannot be negative when converting to std::size_t");
    }
    static_assert(sizeof(std::size_t) >= sizeof(value));
    return static_cast<std::size_t>(value);
}

constexpr logicitem_key_t::operator difference_type() const {
    static_assert(sizeof(difference_type) >= sizeof(value));
    return difference_type {value};
}

constexpr logicitem_key_t::operator bool() const noexcept {
    return value >= 0;
}

constexpr auto logicitem_key_t::max() noexcept -> logicitem_key_t {
    return logicitem_key_t {std::numeric_limits<value_type>::max()};
};

constexpr auto logicitem_key_t::operator++() -> logicitem_key_t & {
    if (value < value_type {0}) [[unlikely]] {
        throw std::runtime_error("logicitem_key cannot be negative when incrementing");
    }
    if (value >= max().value) [[unlikely]] {
        throw std::overflow_error("cannot increment, overflow");
    }
    ++value;
    return *this;
}

constexpr auto logicitem_key_t::operator++(int) -> logicitem_key_t {
    auto tmp = *this;
    operator++();
    return tmp;
}

//
// Constants
//

constexpr inline static auto null_logicitem_key = logicitem_key_t {-1};
static_assert(null_logicitem_key == logicitem_key_t {});

}  // namespace logicsim

//
// Hash function
//

template <>
struct ankerl::unordered_dense::hash<logicsim::logicitem_key_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::logicitem_key_t &obj) const noexcept
        -> uint64_t {
        return logicsim::wyhash(obj.value);
    }
};

#endif
