#ifndef LOGICSIM_ALGORITHM_SHUFFLE_H
#define LOGICSIM_ALGORITHM_SHUFFLE_H

#include "core/algorithm/uniform_int_distribution.h"

#include <algorithm>
#include <iterator>
#include <random>
#include <ranges>

namespace logicsim {

/**
 * @brief Shuffles all elements of the given container randomly.
 *
 * Many shuffle algorithms is not portable, meaning giving the same
 * results on all platforms. This is why we implement it ourselves:
 *   * std::shuffle
 *   * ranges::actions::shuffle uses std::uniform_int_distribution
 *   * boost::ranges::algorithm::random_shuffle uses std::shuffle
 *
 * Algorithm Fisher-Yates shuffle, copied from std::shuffle.
 */
template <std::random_access_iterator Iterator,
          std::uniform_random_bit_generator Generator>
void shuffle(const Iterator first, const Iterator last, Generator& generator) {
    if (first == last) {
        return;
    }
    using diff_t = std::iter_difference_t<Iterator>;

    auto pivot = first + 1;
    diff_t pivot_index = 1;

    for (; pivot != last; ++pivot, ++pivot_index) {
        const auto distribution = uint_distribution(diff_t {0}, pivot_index);
        diff_t offset_index = distribution(generator);

        if (offset_index != pivot_index) {  // avoid self-move-assignment
            std::iter_swap(pivot, first + offset_index);
        }
    }
}

/// @brief Shuffles all elements of the given container randomly.
template <std::ranges::random_access_range Range,
          std::uniform_random_bit_generator Generator>
void shuffle(Range& range, Generator& generator) {
    shuffle(std::ranges::begin(range), std::ranges::end(range), generator);
}
}  // namespace logicsim

#endif
