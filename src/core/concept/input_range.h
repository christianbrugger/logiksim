#ifndef LOGICSIM_CONCEPT_INPUT_RANGE_H
#define LOGICSIM_CONCEPT_INPUT_RANGE_H

#include <ranges>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Constraints R to an input range with elements of specific type V.
 */
template <typename R, typename V>
concept input_range_of = std::ranges::input_range<R> &&
                         std::convertible_to<std::ranges::range_reference_t<R>, V>;

/**
 * @brief: Constraints R to an input range with elements of specific type V1 or V2.
 */
template <typename R, typename V1, typename V2>
concept input_range_of2 = std::ranges::input_range<R> &&
                          (std::convertible_to<std::ranges::range_reference_t<R>, V1> ||
                           std::convertible_to<std::ranges::range_reference_t<R>, V2>);

}  // namespace logicsim

#endif
