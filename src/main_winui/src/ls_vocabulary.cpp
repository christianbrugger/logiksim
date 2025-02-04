#include "pch.h"

#include "main_winui/src/ls_vocabulary.h"

#include <cmath>
#include <concepts>
#include <limits>

namespace logicsim {

namespace {

/**
 * @brief: Round the value and then casts it to an integral.
 */
template <std::integral T>
[[nodiscard]] auto saturate_round_cast(double orig) -> T {
    // make sure max & min fit into double
    static_assert(std::numeric_limits<double>::digits > std::numeric_limits<T>::digits);
    constexpr auto min_int = std::numeric_limits<T>::lowest();
    constexpr auto max_int = std::numeric_limits<T>::max();

    const auto trunc = std::round(orig);

    if (std::isnan(trunc)) {
        return 0;
    }

    if (trunc <= double {min_int}) {
        return min_int;
    }
    if (trunc >= double {max_int}) {
        return max_int;
    }

    // See: Floating-integral conversions
    // https://en.cppreference.com/w/cpp/language/implicit_conversion

    return static_cast<T>(trunc);
}

}  // namespace

auto to_point_pixel_int(const PointPixel& point) -> PointPixelInt {
    return PointPixelInt {
        .x = saturate_round_cast<int32_t>(point.x),
        .y = saturate_round_cast<int32_t>(point.y),
    };
}

}  // namespace logicsim
