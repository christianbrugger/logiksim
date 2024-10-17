#ifndef LOGICSIM_ALGORITHM_NARROW_INTEGRAL_H
#define LOGICSIM_ALGORITHM_NARROW_INTEGRAL_H

#include "core/concept/integral.h"

#include <gsl/gsl>

#include <type_traits>

namespace logicsim {

/**
 * @brief: Narrows the given integral type to the target integral type T.
 *
 * Removes unecesary checks for conversions that do not require narrowing.
 */
template <integral T>
constexpr auto narrow_integral(integral auto value) -> T {
    if constexpr (sizeof(decltype(value)) < sizeof(T) ||
                  std::is_same_v<decltype(value), T>) {
        return T {value};
    } else {
        return gsl::narrow<T>(value);
    }
}

}  // namespace logicsim

#endif
