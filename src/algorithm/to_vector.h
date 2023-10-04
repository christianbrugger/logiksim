#ifndef LOGICSIM_ALGORITHM_TO_VECTOR_H
#define LOGICSIM_ALGORITHM_TO_VECTOR_H

#include <ranges>
#include <vector>

namespace logicsim {

/**
 * brief: Convert the range to a std::vector
 */
template <std::ranges::input_range R>
constexpr auto to_vector(R&& range) {
    return std::vector(std::begin(range), std::end(range));
}

}  // namespace logicsim

#endif
