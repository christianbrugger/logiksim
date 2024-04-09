#ifndef LOGICSIM_ALGORITHM_MERGED_NONE_OF_H
#define LOGICSIM_ALGORITHM_MERGED_NONE_OF_H

#include <iterator>
#include <ranges>

namespace logicsim {

template <std::input_iterator I1, std::sentinel_for<I1> S1, std::input_iterator I2,
          std::sentinel_for<I2> S2, class Comp = std::ranges::less,
          std::indirect_binary_predicate<I1, I2> Pred = std::ranges::equal_to>
auto merged_none_of(I1 first1, S1 last1, I2 first2, S2 last2, Comp comp = {},
                    Pred pred = {}) -> bool {
    while (first1 != last1 && first2 != last2) {
        if (std::invoke(pred, *first1, *first2)) {
            return false;
        }

        if (std::invoke(comp, *first1, *first2)) {
            ++first1;
        } else {
            ++first2;
        }
    }

    return true;
}

template <std::ranges::input_range R1, std::ranges::input_range R2,
          class Comp = std::ranges::less,
          std::indirect_binary_predicate<std::ranges::iterator_t<R1>,
                                         std::ranges::iterator_t<R2>>
              Pred = std::ranges::equal_to>
auto merged_none_of(R1&& r1, R2&& r2, Comp comp = {}, Pred pred = {}) -> bool {
    return merged_none_of(std::ranges::begin(r1), std::ranges::end(r1),
                          std::ranges::begin(r2), std::ranges::end(r2), std::move(comp),
                          std::move(pred));
}

}  // namespace logicsim

#endif
