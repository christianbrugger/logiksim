#ifndef LOGICSIM_VOCABULARY_DEVICE_PIXEL_RATIO_H
#define LOGICSIM_VOCABULARY_DEVICE_PIXEL_RATIO_H

#include "core/format/struct.h"

#include <compare>
#include <string>

namespace logicsim {

/**
 * @brief: A continuous location on the grid in one dimension.
 */
struct device_pixel_ratio_t {
    using value_type = double;
    value_type value {0.};

    [[nodiscard]] explicit constexpr device_pixel_ratio_t() = default;
    [[nodiscard]] explicit constexpr device_pixel_ratio_t(value_type v) noexcept;

    [[nodiscard]] explicit constexpr operator double() const noexcept;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] constexpr auto operator==(const device_pixel_ratio_t &other) const
        -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const device_pixel_ratio_t &other) const =
        default;
};

static_assert(std::regular<device_pixel_ratio_t>);

static_assert(std::is_trivially_copyable_v<device_pixel_ratio_t>);
static_assert(std::is_trivially_copy_assignable_v<device_pixel_ratio_t>);

//
// Implementation
//

constexpr device_pixel_ratio_t::device_pixel_ratio_t(value_type v) noexcept : value {v} {}

constexpr device_pixel_ratio_t::operator double() const noexcept {
    return double {value};
}
}  // namespace logicsim

#endif
