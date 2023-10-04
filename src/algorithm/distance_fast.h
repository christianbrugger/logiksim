#ifndef LOGICSIM_ALGORITHM_DISTANCE_FAST_H
#define LOGICSIM_ALGORITHM_DISTANCE_FAST_H

#include <iterator>
#include <ranges>

namespace logicsim {

/**
 * @brief: Calculate distance in O(1) of a range.
 */
template <typename IteratorFirst, typename IteratorLast>
    requires std::sized_sentinel_for<IteratorLast, IteratorFirst>
auto distance_fast(IteratorFirst first, IteratorLast last) {
    return last - first;
}

/**
 * @brief: Calculate distance in O(1) of a range.
 */
auto distance_fast(std::ranges::input_range auto&& range) {
    return distance_fast(std::begin(range), std::end(range));
}

}  // namespace logicsim

#endif
