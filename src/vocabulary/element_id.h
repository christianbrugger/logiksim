#ifndef LOGIKSIM_VOCABULARY_ELEMENT_ID_H
#define LOGIKSIM_VOCABULARY_ELEMENT_ID_H

#include "difference_type.h"
#include "format/struct.h"
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
    [[nodiscard]] explicit constexpr element_id_t(value_type value_) noexcept;
    [[nodiscard]] explicit constexpr element_id_t(unsigned int value_);
    [[nodiscard]] explicit constexpr element_id_t(long value_);
    [[nodiscard]] explicit constexpr element_id_t(unsigned long value_);
    [[nodiscard]] explicit constexpr element_id_t(long long value_);
    [[nodiscard]] explicit constexpr element_id_t(unsigned long long value_);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] explicit constexpr operator bool() const noexcept;
    [[nodiscard]] auto operator==(const element_id_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const element_id_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept;

    constexpr auto operator++() noexcept -> element_id_t &;
    constexpr auto operator++(int) noexcept -> element_id_t;
};

static_assert(std::is_trivial_v<element_id_t>);

//
// Implementation
//

constexpr element_id_t::element_id_t(value_type value_) noexcept : value {value_} {};

constexpr element_id_t::element_id_t(unsigned int value_)
    : value {gsl::narrow<value_type>(value_)} {};

constexpr element_id_t::element_id_t(long value_)
    : value {gsl::narrow<value_type>(value_)} {};

constexpr element_id_t::element_id_t(unsigned long value_)
    : value {gsl::narrow<value_type>(value_)} {};

constexpr element_id_t::element_id_t(long long value_)
    : value {gsl::narrow<value_type>(value_)} {};

constexpr element_id_t::element_id_t(unsigned long long value_)
    : value {gsl::narrow<value_type>(value_)} {};

constexpr element_id_t::operator bool() const noexcept {
    return value >= 0;
}

constexpr auto element_id_t::max() noexcept {
    return std::numeric_limits<value_type>::max();
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
