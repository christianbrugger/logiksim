#ifndef LOGICSIM_ALGORITHM_ROUND_H
#define LOGICSIM_ALGORITHM_ROUND_H

#include <gsl/gsl>

#include <cassert>
#include <cfenv>
#include <cmath>
#include <limits>

namespace logicsim {

inline auto correct_round_mode() {
    return std::fegetround() == FE_TONEAREST;
}

/**
 * @brief: Fast floating point rounding.
 */
inline auto round_fast(double value) -> double {
    // std::nearbyint is faster than std::round, but we need to check rounding mode
    assert(correct_round_mode());
    return std::nearbyint(value);
}

/**
 * @brief: Round a value to a specific type. The conversion is checked.
 */
template <typename result_type = double>
auto round_to(double value) -> result_type {
    return gsl::narrow<result_type>(round_fast(value));
}

/**
 * @brief: Round and clamp a value to a type.
 */
template <typename result_type = double>
auto clamp_to(double value) -> result_type {
    constexpr static double max =
        gsl::narrow<double>(std::numeric_limits<result_type>::max());
    constexpr static double min =
        gsl::narrow<double>(std::numeric_limits<result_type>::lowest());

    // we don't handle NaN for now, as its not needed
    if (value >= max) {
        return std::numeric_limits<result_type>::max();
    }
    if (value <= min) {
        return std::numeric_limits<result_type>::lowest();
    }

    return gsl::narrow_cast<result_type>(value);
}

}  // namespace logicsim

#endif
