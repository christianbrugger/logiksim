#ifndef LOGICSIM_VOCABULARY_POINT_DEVICE_FINE_H
#define LOGICSIM_VOCABULARY_POINT_DEVICE_FINE_H
#include "format/struct.h"

#include <compare>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Point in device coordinates (scaled by device pixel ratio).
 */
struct point_device_fine_t {
    using value_type = double;

    value_type x;
    value_type y;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const point_device_fine_t &other) const
        -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const point_device_fine_t &other) const =
        default;
};

static_assert(std::is_trivial_v<point_device_fine_t>);

}  // namespace logicsim

#endif
