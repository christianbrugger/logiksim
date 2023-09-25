#ifndef LOGIKSIM_VOCABULARY_ELEMENT_ID_H
#define LOGIKSIM_VOCABULARY_ELEMENT_ID_H

#include "format/struct.h"

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

    using difference_type = value_type;
    static_assert(std::is_signed_v<difference_type>);

    explicit constexpr element_id_t() = default;
    explicit constexpr element_id_t(value_type value_) noexcept : value {value_} {};

    explicit constexpr element_id_t(unsigned int value_)
        : value {gsl::narrow<value_type>(value_)} {};
    explicit constexpr element_id_t(long value_)
        : value {gsl::narrow<value_type>(value_)} {};
    explicit constexpr element_id_t(unsigned long value_)
        : value {gsl::narrow<value_type>(value_)} {};
    explicit constexpr element_id_t(long long value_)
        : value {gsl::narrow<value_type>(value_)} {};
    explicit constexpr element_id_t(unsigned long long value_)
        : value {gsl::narrow<value_type>(value_)} {};

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const element_id_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const element_id_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept {
        return std::numeric_limits<value_type>::max();
    };

    [[nodiscard]] explicit constexpr operator bool() const noexcept {
        return value >= 0;
    }

    constexpr auto operator++() noexcept -> element_id_t & {
        ++value;
        return *this;
    }

    constexpr auto operator++(int) noexcept -> element_id_t {
        auto tmp = *this;
        operator++();
        return tmp;
    }
};

static_assert(std::is_trivial<element_id_t>::value);

constexpr inline static auto null_element = element_id_t {-1};

}  // namespace logicsim

#endif
