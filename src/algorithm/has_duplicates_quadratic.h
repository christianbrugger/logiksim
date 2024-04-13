#ifndef LOGICSIM_ALGORITHM_HAS_DUPLICATES_QUADRATIC_H
#define LOGICSIM_ALGORITHM_HAS_DUPLICATES_QUADRATIC_H

#include <functional>
#include <iterator>
#include <ranges>
#include <utility>

namespace logicsim {

/**
 * brief: Callable that returns false.
 */
struct always_false {
    [[nodiscard]] constexpr auto operator()(auto&& left [[maybe_unused]], auto&& right
                                            [[maybe_unused]]) const noexcept -> bool {
        return false;
    }
};

/**
 * brief: Check if container contains duplicates.
 *
 * Note this algorithm is O(n^2). Only good for very small sizes.
 */
template <class Proj = std::identity, class Comp = std::equal_to<>,
          class Ignore = always_false>
auto has_duplicates_quadratic(std::bidirectional_iterator auto begin,
                              std::bidirectional_iterator auto end, Proj proj = {},
                              Comp comp = {}, Ignore ignore = {}) -> bool {
    if (begin == end) {
        return false;
    }
    for (auto i1 = begin; i1 != std::prev(end); ++i1) {
        for (auto i2 = std::next(i1); i2 != end; ++i2) {
            if (ignore(i1, i2)) {
                continue;
            }
            if (comp(std::invoke(proj, *i1), std::invoke(proj, *i2))) {
                return true;
            }
        }
    }
    return false;
}

/**
 * brief: Check if container contains duplicates.
 *
 * Note this algorithm is O(n^2). Only good for very small sizes.
 */
template <class Proj = std::identity, class Comp = std::equal_to<>,
          class Ignore = always_false>
auto has_duplicates_quadratic(std::ranges::input_range auto&& range, Proj proj = {},
                              Comp comp = {}, Ignore ignore = {}) -> bool{
    return has_duplicates_quadratic(std::ranges::begin(range), std::ranges::end(range),
                                    std::move(proj), std::move(comp), std::move(ignore));
}

/**
 * brief: Check if container contains duplicates.
 *
 * Note comparison receives the iterators itself, not the values.
 *
 * Note this algorithm is O(n^2). Only good for very small sizes.
 */
template <class Comp>
auto has_duplicates_quadratic_iterator(std::bidirectional_iterator auto begin,
                                       std::bidirectional_iterator auto end, Comp comp)
    -> bool {
    if (begin == end) {
        return false;
    }
    for (auto i1 = begin; i1 != std::prev(end); ++i1) {
        for (auto i2 = std::next(i1); i2 != end; ++i2) {
            if (comp(i1, i2)) {
                return true;
            }
        }
    }
    return false;
}

}  // namespace logicsim

#endif
