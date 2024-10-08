#ifndef LOGICSIM_ALGORITHM_RANGE_EXTENDED_H
#define LOGICSIM_ALGORITHM_RANGE_EXTENDED_H

#include "concept/explicitly_convertible.h"

#include <gsl/gsl>

#include <cassert>
#include <limits>
#include <ranges>
#include <type_traits>
#include <utility>  // std::size_t

namespace logicsim {

/**
 * @brief: Range of values from [start, .. end) converted to T via value_type.
 *
 * Expects:
 *      * value_type & T can hold all number [start, .. end)
 */
template <typename T, typename value_type = typename T::value_type>
    requires explicitly_convertible_to<std::size_t, value_type> &&
             std::is_constructible_v<T, value_type>
constexpr auto range_extended(std::size_t start, std::size_t end) {
    Expects(end == 0 ||
            end - 1 <= gsl::narrow<std::size_t>(std::numeric_limits<value_type>::max()));

    return std::ranges::views::transform(
        std::ranges::views::iota(start, end),
        [](const std::size_t &v) -> T { return T {static_cast<value_type>(v)}; });
}

/**
 * @brief: Range of values from [first, .. last] converted to T via value_type.
 *
 * Expects:
 *      * value_type & T can hold all number [first, .. last]
 */
template <typename T, typename value_type = typename T::value_type>
    requires explicitly_convertible_to<std::size_t, value_type> &&
             std::is_constructible_v<T, value_type>
constexpr auto range_inclusive(std::size_t first, std::size_t last) {
    Expects(last <= gsl::narrow<std::size_t>(std::numeric_limits<value_type>::max()));
    Expects(last < std::numeric_limits<std::size_t>::max());

    return std::ranges::views::transform(
        std::ranges::views::iota(first, last + std::size_t {1}),
        [](const std::size_t &v) -> T { return T {static_cast<value_type>(v)}; });
}

/**
 * @brief: Range of values from [0, .. count) converted to T via value_type.
 *
 * Expects:
 *      * value_type & T can hold all number [0, .. count)
 */
template <typename T, typename value_type = typename T::value_type>
    requires explicitly_convertible_to<std::size_t, value_type> &&
             std::is_constructible_v<T, value_type>
constexpr auto range_extended(std::size_t count) {
    return range_extended<T, value_type>(std::size_t {0}, count);
}

/**
 * @brief: Returned type of range_extended
 */
template <typename T, typename value_type = typename T::value_type>
    requires explicitly_convertible_to<std::size_t, value_type> &&
                 std::is_constructible_v<T, value_type>
using range_extended_t = decltype(range_extended<T, value_type>(0));

}  // namespace logicsim

#endif
