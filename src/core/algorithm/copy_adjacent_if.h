#ifndef LOGICSIM_ALGORITHM_COPY_ADJACENT_IF_H
#define LOGICSIM_ALGORITHM_COPY_ADJACENT_IF_H

#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>

namespace logicsim {

/**
 * brief: Copies values if adjacent values fulfil predicate.
 *
 * Note that the copied values are being projected.
 */
template <std::forward_iterator I, std::sentinel_for<I> S, std::weakly_incrementable O,
          class Proj = std::identity,
          std::indirect_binary_predicate<std::projected<I, Proj>, std::projected<I, Proj>>
              Pred = std::ranges::equal_to>
    requires std::indirectly_writable<O, std::indirect_result_t<Proj&, I>>
constexpr auto copy_adjacent_if(I first, S last, O result, Pred pred = {},
                                Proj proj = {}) -> void {
    auto&& it = first;

    while (it != last) {
        it = std::ranges::adjacent_find(it, last, pred, proj);

        if (it != last) {
            result = std::invoke(proj, *it);
            ++result;
            ++it;
        }
    }
}

}  // namespace logicsim

#endif
