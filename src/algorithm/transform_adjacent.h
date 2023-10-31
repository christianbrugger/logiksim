#ifndef LOGICSIM_ALGORITHM_TRANSFORM_ADJACENT_H
#define LOGICSIM_ALGORITHM_TRANSFORM_ADJACENT_H

#include <algorithm>
#include <iterator>
#include <ranges>

namespace logicsim {

template <std::input_iterator I, std::sentinel_for<I> S, std::weakly_incrementable O,
          std::copy_constructible F, class Proj = std::identity>
    requires std::indirectly_writable<
        O, std::indirect_result_t<F&, std::projected<I, Proj>, std::projected<I, Proj>>>
constexpr auto transform_adjacent(I first, S last, O result, F binary_op,
                                  Proj proj = {}) {
    if (first == last) {
        return;
    }
    std::ranges::transform(first, last, std::next(first), last, result, binary_op, proj,
                           proj);
}

template <std::ranges::input_range R, std::weakly_incrementable O,
          std::copy_constructible F, class Proj = std::identity>
    requires std::indirectly_writable<
        O, std::indirect_result_t<F&, std::projected<std::ranges::iterator_t<R>, Proj>,
                                  std::projected<std::ranges::iterator_t<R>, Proj>>>
constexpr auto transform_adjacent(R&& r, O result, F binary_op, Proj proj = {}) {
    return transform(std::ranges::begin(r), std::ranges::end(r), result, binary_op, proj);
}

}  // namespace logicsim

#endif
