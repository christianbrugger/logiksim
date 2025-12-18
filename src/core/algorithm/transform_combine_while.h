#ifndef LOGICSIM_ALGORITHM_TRANSFORM_COMBINE_WHILE_H
#define LOGICSIM_ALGORITHM_TRANSFORM_COMBINE_WHILE_H

#include <functional>
#include <iterator>
#include <ranges>

namespace logicsim {

template <class IteratorFirst, class IteratorLast, class OutputIterator, class MakeState,
          class Pred, class Update = std::identity, class Proj = std::identity>
auto transform_combine_while(IteratorFirst first, IteratorLast last,
                             OutputIterator result, MakeState make_state, Pred predicate,
                             Update update, Proj project) -> OutputIterator {
    auto i0 = first;

    while (i0 != last) {
        auto i1 = i0 + 1;
        auto state = make_state(i0);

        while (i1 != last && predicate(state, i1)) {
            state = update(state, i1);
            ++i1;
        }

        *result = project(state);
        ++result;

        i0 = i1;
    }

    return result;
}

template <std::ranges::input_range R, class OutputIterator, class MakeState, class Pred,
          class Update = std::identity, class Proj = std::identity>
auto transform_combine_while(const R& r, OutputIterator result, MakeState make_state,
                             Pred predicate, Update update = {}, Proj project = {})
    -> OutputIterator {
    return transform_combine_while(std::begin(r), std::end(r), result, make_state,
                                   predicate, update, project);
}

}  // namespace logicsim

#endif
