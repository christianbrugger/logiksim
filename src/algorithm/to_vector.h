#ifndef LOGICSIM_ALGORITHM_TO_VECTOR_H
#define LOGICSIM_ALGORITHM_TO_VECTOR_H

#include <algorithm>
#include <iterator>
#include <ranges>
#include <vector>

namespace logicsim {

/**
 * brief: Convert the range to a std::vector
 */
template <std::ranges::input_range R>
    requires std::ranges::sized_range<R>
constexpr auto to_vector(R&& range) {
    using iterator = std::ranges::iterator_t<R>;
    using value_type = std::iter_value_t<iterator>;

    auto result = std::vector<value_type> {};
    result.reserve(std::ranges::size(range));

    std::ranges::copy(range, std::back_inserter(result));
    return result;
}

}  // namespace logicsim

#endif
