#ifndef LOGICSIM_VOCABULARY_POINT_DEVICE_H
#define LOGICSIM_VOCABULARY_POINT_DEVICE_H
#include "format/struct.h"

#include <compare>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Point in device coordinates (scaled by device pixel ratio).
 */
struct point_device_t {
    using value_type = int;

    value_type x {0};
    value_type y {0};

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const point_device_t &other) const
        -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const point_device_t &other) const = default;
};

static_assert(std::is_aggregate_v<point_device_t>);
static_assert(std::is_trivially_copyable_v<point_device_t>);

}  // namespace logicsim

#endif
