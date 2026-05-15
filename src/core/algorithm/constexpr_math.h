#ifndef LOGICSIM_ALGORITHM_CONTAINS_H
#define LOGICSIM_ALGORITHM_CONTAINS_H

#include <gcem.hpp>

#include <cmath>

namespace logicsim {

namespace cmath {

[[nodiscard]] constexpr auto abs(double x) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::abs(x);
    }
    return std::abs(x);
}

[[nodiscard]] constexpr auto pow(double x, double y) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::pow(x, y);
    }
    return std::pow(x, y);
}

[[nodiscard]] constexpr auto cbrt(double x) -> double {
    if (std::is_constant_evaluated()) {
        if (x == 0) {
            return 0;
        }
        // use double to get full double precision
        if (x < 0) {
            return -gcem::pow(-x, 1.0 / 3.0);
        }
        return gcem::pow(x, 1.0 / 3.0);
    }
    return std::cbrt(x);
}

[[nodiscard]] constexpr auto sqrt(double x) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::sqrt(x);
    }
    return std::sqrt(x);
}

[[nodiscard]] constexpr auto hypot(double x, double y) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::hypot(x, y);
    }
    return std::hypot(x, y);
}

[[nodiscard]] constexpr auto hypot(double x, double y, double z) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::sqrt(x * x + y * y + z * z);
    }
    return std::hypot(x, y, z);
}

[[nodiscard]] constexpr auto sin(double x) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::sin(x);
    }
    return std::sin(x);
}

[[nodiscard]] constexpr auto cos(double x) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::cos(x);
    }
    return std::cos(x);
}

[[nodiscard]] constexpr auto atan(double x) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::atan(x);
    }
    return std::atan(x);
}

[[nodiscard]] constexpr auto atan2(double y, double x) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::atan2(y, x);
    }
    return std::atan2(y, x);
}

[[nodiscard]] constexpr auto fmod(double x, double y) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::fmod(x, y);
    }
    return std::fmod(x, y);
}

[[nodiscard]] constexpr auto log(double x) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::log(x);
    }
    return std::log(x);
}

/*
[[nodiscard]] constexpr auto isfinite(double x) -> bool {
    if (std::is_constant_evaluated()) {
        return gcem::isfinite(x);
    }
    return std::isfinite(x);
}
*/

[[nodiscard]] constexpr auto floor(double x) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::floor(x);
    }
    return std::floor(x);
}

[[nodiscard]] constexpr auto ceil(double x) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::ceil(x);
    }
    return std::ceil(x);
}

[[nodiscard]] constexpr auto clamp(double x, double min, double max) -> double {
    if (x < min) {
        return min;
    }
    if (x > max) {
        return max;
    }

    return x;
}

[[nodiscard]] constexpr auto sgn(double x) -> double {
    return static_cast<double>(0 < x) - static_cast<double>(x < 0);
}

}  // namespace cmath

}  // namespace logicsim

#endif
