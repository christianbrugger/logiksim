#ifndef LOGICSIM_ALGORITHM_RANGE_EXTENDED_H
#define LOGICSIM_ALGORITHM_RANGE_EXTENDED_H

#include "core/concept/explicitly_convertible.h"

#include <gsl/gsl>

#include <cassert>
#include <limits>
#include <ranges>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Range of values from [start, .. end) converted to T via value_type.
 *
 * Expects:
 *      * value_type & T can hold all number [start, .. end)
 */
template <typename T, typename value_type = typename T::value_type>
    requires explicitly_convertible_to<std::ptrdiff_t, value_type> &&
             std::is_constructible_v<T, value_type>
constexpr auto range_extended(std::ptrdiff_t start, std::ptrdiff_t end) {
    Expects(end == 0 || end - 1 <= gsl::narrow<std::ptrdiff_t>(
                                       std::numeric_limits<value_type>::max()));

    return std::ranges::views::transform(
        std::ranges::views::iota(start, end),
        [](const std::ptrdiff_t &v) -> T { return T {static_cast<value_type>(v)}; });
}

/**
 * @brief: Range of values from [first, .. last] converted to T via value_type.
 *
 * Expects:
 *      * value_type & T can hold all number [first, .. last]
 */
template <typename T, typename value_type = typename T::value_type>
    requires explicitly_convertible_to<std::ptrdiff_t, value_type> &&
             std::is_constructible_v<T, value_type>
constexpr auto range_inclusive(std::ptrdiff_t first, std::ptrdiff_t last) {
    Expects(last <= gsl::narrow<std::ptrdiff_t>(std::numeric_limits<value_type>::max()));
    Expects(last < std::numeric_limits<std::ptrdiff_t>::max());

    return std::ranges::views::transform(
        std::ranges::views::iota(first, last + std::ptrdiff_t {1}),
        [](const std::ptrdiff_t &v) -> T { return T {static_cast<value_type>(v)}; });
}

/**
 * @brief: Range of values from [0, .. count) converted to T via value_type.
 *
 * Expects:
 *      * value_type & T can hold all number [0, .. count)
 */
template <typename T, typename value_type = typename T::value_type>
    requires explicitly_convertible_to<std::ptrdiff_t, value_type> &&
             std::is_constructible_v<T, value_type>
constexpr auto range_extended(std::ptrdiff_t count) {
    return range_extended<T, value_type>(std::ptrdiff_t {0}, count);
}

/**
 * @brief: Returned type of range_extended
 */
template <typename T, typename value_type = typename T::value_type>
    requires explicitly_convertible_to<std::ptrdiff_t, value_type> &&
                 std::is_constructible_v<T, value_type>
using range_extended_t = decltype(range_extended<T, value_type>(0));

}  // namespace logicsim

#endif
