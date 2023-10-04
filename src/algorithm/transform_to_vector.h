#ifndef LOGICSIM_ALGORITHM_TRANSFORM_TO_VECTOR_H
#define LOGICSIM_ALGORITHM_TRANSFORM_TO_VECTOR_H

#include "algorithm/transform_to_container.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>
#include <utility>
#include <vector>

namespace logicsim {

/**
 * brief: Stores the transformed values of the range to a new std::vector.
 */
template <std::input_iterator InputIt, class Function>
    requires std::sized_sentinel_for<InputIt, InputIt>
constexpr auto transform_to_vector(InputIt first, InputIt last, Function func) {
    using result_type = std::decay_t<decltype(std::invoke(func, *first))>;
    using container_type = std::vector<result_type>;

    return transform_to_container<container_type>(first, last, func);
}

/**
 * brief: Stores the transformed values of the range to a new std::vector.
 */
template <class Function>
constexpr auto transform_to_vector(std::ranges::input_range auto&& range, Function func) {
    return transform_to_vector(std::ranges::begin(range), std::ranges::end(range), func);
}

}  // namespace logicsim

#endif
