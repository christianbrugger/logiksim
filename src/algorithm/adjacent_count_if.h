#ifndef LOGICSIM_ALGORITHM_ADJACENT_COUNT_IF_H
#define LOGICSIM_ALGORITHM_ADJACENT_COUNT_IF_H

#include <functional>  // std::invoke, std::identity
#include <iterator>
#include <ranges>

namespace logicsim {

/**
 * brief: Count all adjacent elements that fulfill binary predicate after being projected.
 */
template <std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
          std::indirect_binary_predicate<std::projected<I, Proj>, std::projected<I, Proj>>
              Pred = std::ranges::equal_to>
constexpr auto adjacent_count_if(I first, S last, Pred pred = {}, Proj proj = {})
    -> std::iter_difference_t<I> {
    static_assert(std::input_iterator<I>);
    static_assert(std::sentinel_for<I, S>);
    static_assert(std::indirect_binary_predicate<Pred, std::projected<I, Proj>,
                                                 std::projected<I, Proj>>);

    auto count = std::iter_difference_t<I> {0};
    if (first == last) {
        return count;
    }

    auto next = first;
    ++next;
    for (; next != last; ++next, ++first) {
        if (std::invoke(pred, std::invoke(proj, *first), std::invoke(proj, *next))) {
            ++count;
        }
    }
    return count;
}

/**
 * brief: Count all adjacent elements that fulfill binary predicate after being projected.
 */
template <std::ranges::forward_range R, class Proj = std::identity,
          std::indirect_binary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>,
                                         std::projected<std::ranges::iterator_t<R>, Proj>>
              Pred = std::ranges::equal_to>
constexpr auto adjacent_count_if(const R& r, Pred pred = {}, Proj proj = {}) {
    return adjacent_count_if(std::ranges::begin(r), std::ranges::end(r), std::ref(pred),
                             std::ref(proj));
}

}  // namespace logicsim

#endif
