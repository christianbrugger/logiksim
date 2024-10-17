#ifndef LOGICSIM_ALGORITHM_RANDOM_SELECT_H
#define LOGICSIM_ALGORITHM_RANDOM_SELECT_H

#include "core/algorithm/uniform_int_distribution.h"

#include <iterator>
#include <random>
#include <ranges>

namespace logicsim {

/**
 * brief: Select a random element of the iterator pair.
 */
template <std::random_access_iterator Iterator,
          std::uniform_random_bit_generator Generator>
[[nodiscard]] auto random_select(const Iterator first, const Iterator last,
                                 Generator& generator) -> Iterator {
    if (first == last) {
        return last;
    }

    const auto first_index = typename std::iterator_traits<Iterator>::difference_type {0};
    const auto last_index = std::distance(first, std::prev(last));

    const auto index = uint_distribution(first_index, last_index)(generator);

    return std::next(first, index);
}

/**
 * brief: Select a random element of the given range.
 */
template <std::ranges::random_access_range Range,
          std::uniform_random_bit_generator Generator>
auto random_select(Range& range, Generator& generator) {
    return random_select(std::ranges::begin(range), std::ranges::end(range), generator);
}

}  // namespace logicsim

#endif
