
#ifndef LOGICSIM_ALGORITHM_GOLDEN_MINIMIZE_H
#define LOGICSIM_ALGORITHM_GOLDEN_MINIMIZE_H

#include "core/algorithm/constexpr_math.h"

#include <gsl/gsl>

#include <limits>
#include <numbers>
#include <stdexcept>

namespace logicsim {

constexpr auto golden_minimize_iter_count(int digits) -> int {
    constexpr double inv_phi = 1. / std::numbers::phi;

    return static_cast<int>(cmath::ceil(digits * cmath::log(0.5) / cmath::log(inv_phi)));
}

namespace defaults {
constexpr inline static auto golden_minimize_count =
    golden_minimize_iter_count(std::numeric_limits<double>::digits);
}

template <typename Func>
[[nodiscard]] constexpr auto golden_minimize(
    Func func, double a, double b, int iter_count = defaults::golden_minimize_count)
    -> std::pair<double, double> {
    constexpr double inv_phi = 1. / std::numbers::phi;

    if (iter_count < 0) [[unlikely]] {
        throw std::runtime_error {"iter_count cannot be negative"};
    }
    if (a > b) [[unlikely]] {
        throw std::runtime_error {"a needs to be smaller or equal to b"};
    }

    double x1 = b - inv_phi * (b - a);
    double x2 = a + inv_phi * (b - a);
    double f1 = func(x1);
    double f2 = func(x2);

    for (int i = 0; i < iter_count; ++i) {
        if (f1 > f2) {
            a = x1;
            x1 = x2;
            f1 = f2;
            x2 = a + inv_phi * (b - a);
            f2 = func(x2);
        } else {
            b = x2;
            x2 = x1;
            f2 = f1;
            x1 = b - inv_phi * (b - a);
            f1 = func(x1);
        }
    }

    if (f1 < f2) {
        return std::pair {x1, f1};
    }
    return std::pair {x2, f2};
}

}  // namespace logicsim

#endif
