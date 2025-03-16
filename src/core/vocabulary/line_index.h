#ifndef LOGICSIM_VOCABULARY_LINE_INDEX_H
#define LOGICSIM_VOCABULARY_LINE_INDEX_H

#include "core/algorithm/narrow_integral.h"
#include "core/concept/explicitly_convertible.h"
#include "core/concept/integral.h"
#include "core/format/struct.h"
#include "core/type_trait/safe_difference_type.h"

#include <compare>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Identifier to a line in a line tree.
 */
struct line_index_t {
    using value_type = int32_t;
    value_type value {-1};

    using difference_type = safe_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    [[nodiscard]] explicit constexpr line_index_t() = default;
    [[nodiscard]] explicit constexpr line_index_t(integral auto value_);

    [[nodiscard]] explicit constexpr operator std::size_t() const;
    [[nodiscard]] explicit constexpr operator difference_type() const;
    /**
     * @brief: The bool cast tests if this index is valid.
     */
    [[nodiscard]] explicit constexpr operator bool() const noexcept;
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const line_index_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const line_index_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept -> line_index_t;

    constexpr auto operator++() -> line_index_t &;
    constexpr auto operator++(int) -> line_index_t;

    constexpr auto operator--() -> line_index_t &;
    constexpr auto operator--(int) -> line_index_t;
};

static_assert(std::is_trivially_copyable_v<line_index_t>);
static_assert(std::is_trivially_copy_assignable_v<line_index_t>);
static_assert(explicitly_convertible_to<line_index_t, line_index_t::difference_type>);

[[nodiscard]] constexpr auto get_previous(line_index_t index) -> line_index_t;
[[nodiscard]] constexpr auto get_next(line_index_t index) -> line_index_t;

//
// Implementation
//

constexpr line_index_t::line_index_t(integral auto value_)
    : value {narrow_integral<value_type>(value_)} {}

constexpr line_index_t::operator std::size_t() const {
    if (value < value_type {0}) [[unlikely]] {
        throw std::runtime_error(
            "line index cannot be negative when converting to std::size_t");
    }
    return static_cast<std::size_t>(value);
}

constexpr line_index_t::operator difference_type() const {
    return difference_type {value};
}

constexpr line_index_t::operator bool() const noexcept {
    return value >= 0;
}

constexpr auto line_index_t::max() noexcept -> line_index_t {
    return line_index_t {std::numeric_limits<value_type>::max()};
};

constexpr auto line_index_t::operator++() -> line_index_t & {
    if (value < value_type {0}) [[unlikely]] {
        throw std::runtime_error("element id cannot be negative when incrementing");
    }
    if (value >= max().value) [[unlikely]] {
        throw std::overflow_error("cannot increment, overflow");
    }
    ++value;
    return *this;
}

constexpr auto line_index_t::operator++(int) -> line_index_t {
    auto tmp = *this;
    operator++();
    return tmp;
}

constexpr auto line_index_t::operator--() -> line_index_t & {
    if (value <= value_type {0}) [[unlikely]] {
        throw std::runtime_error("cannot decrement, overflow");
    }
    --value;
    return *this;
}

constexpr auto line_index_t::operator--(int) -> line_index_t {
    auto tmp = *this;
    operator--();
    return tmp;
}

constexpr auto get_previous(line_index_t index) -> line_index_t {
    if (index <= line_index_t {0}) [[unlikely]] {
        throw std::runtime_error("has no previous line index");
    }
    --index;
    return index;
}

constexpr auto get_next(line_index_t index) -> line_index_t {
    ++index;
    return index;
}

//
// Constants
//

inline constexpr auto null_line_index = line_index_t {-1};
static_assert(null_line_index == line_index_t {});

}  // namespace logicsim

#endif
