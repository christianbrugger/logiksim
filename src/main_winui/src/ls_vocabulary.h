#pragma once

#include <compare>
#include <cstdint>
#include <stdexcept>

namespace logicsim {

class ShutdownException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct PointDevice {
    using value_t = float;

    value_t x;
    value_t y;

    [[nodiscard]] auto operator==(const PointDevice&) const -> bool = default;
    [[nodiscard]] auto operator<=>(const PointDevice&) const = default;
};

struct PointPixel {
    using value_t = double;

    value_t x;
    value_t y;

    [[nodiscard]] auto operator==(const PointPixel&) const -> bool = default;
    [[nodiscard]] auto operator<=>(const PointPixel&) const = default;
};

struct PointPixelInt {
    using value_t = int32_t;

    value_t x;
    value_t y;

    [[nodiscard]] auto operator==(const PointPixelInt&) const -> bool = default;
    [[nodiscard]] auto operator<=>(const PointPixelInt&) const = default;
};

[[nodiscard]] auto to_point_pixel_int(const PointPixel& point) -> PointPixelInt;

}  // namespace logicsim
