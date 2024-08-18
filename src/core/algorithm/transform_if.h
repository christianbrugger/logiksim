#ifndef LOGICSIM_ALGORITHM_TRANSFORM_IF_H
#define LOGICSIM_ALGORITHM_TRANSFORM_IF_H

#include <functional>
#include <iterator>
#include <ranges>

namespace logicsim {

/**
 * brief: Transform the projected value if the pred is fulfilled.
 */
template <class IteratorFirst, class IteratorLast, class OutputIterator, class Pred,
          class Proj>
auto transform_if(IteratorFirst first1, IteratorLast last1, OutputIterator result,
                  Proj proj, Pred pred) -> OutputIterator {
    while (first1 != last1) {
        if (std::invoke(pred, *first1)) {
            *result = std::invoke(proj, *first1);
            ++result;
        }
        ++first1;
    }
    return result;
}

/**
 * brief: Transform the projected value if the pred is fulfilled.
 */
template <std::ranges::input_range R, class OutputIterator, class Pred, class Proj>
auto transform_if(const R& r, OutputIterator result, Proj proj, Pred pred) -> void {
    transform_if(std::begin(r), std::end(r), result, std::move(proj), std::move(pred));
}

}  // namespace logicsim

#endif
