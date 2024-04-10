#ifndef LOGICSIM_ALGORITHM_ALL_EQUAL_H
#define LOGICSIM_ALGORITHM_ALL_EQUAL_H

#include <algorithm>
#include <iterator>
#include <ranges>

namespace logicsim {

/**
 * brief: Return true if all elements are equal to the given value.
 */
auto all_equal(std::input_iterator auto first, std::input_iterator auto last,
               auto&& value) -> bool {
    return std::all_of(first, last, [&](const auto& item) { return item == value; });
}

/**
 * brief: Return true if all elements are equal.
 */
auto all_equal(std::ranges::input_range auto&& range, auto value) -> bool {
    return all_equal(std::ranges::begin(range), std::ranges::begin(range), value);
}

}  // namespace logicsim

#endif
