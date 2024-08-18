#ifndef LOGICSIM_VOCABULARY_POINT_DEVICE_FINE_H
#define LOGICSIM_VOCABULARY_POINT_DEVICE_FINE_H
#include "format/struct.h"
#include "vocabulary/point_device.h"

#include <compare>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Point in device coordinates (scaled by device pixel ratio).
 */
struct point_device_fine_t {
    using value_type = double;

    value_type x {0};
    value_type y {0};

    [[nodiscard]] explicit constexpr point_device_fine_t() = default;
    [[nodiscard]] explicit constexpr point_device_fine_t(value_type x_,
                                                         value_type y_) noexcept;
    [[nodiscard]] explicit constexpr point_device_fine_t(point_device_t p) noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const point_device_fine_t &other) const
        -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const point_device_fine_t &other) const =
        default;
};

static_assert(std::is_trivially_copyable_v<point_device_fine_t>);

//
// Implementation
//

constexpr point_device_fine_t::point_device_fine_t(value_type x_, value_type y_) noexcept
    : x {x_}, y {y_} {}

constexpr point_device_fine_t::point_device_fine_t(point_device_t p) noexcept
    : x {static_cast<value_type>(p.x)}, y {static_cast<value_type>(p.y)} {
    static_assert(sizeof(value_type) > sizeof(decltype(p.x)));
    static_assert(sizeof(value_type) > sizeof(decltype(p.y)));
}

}  // namespace logicsim

#endif
