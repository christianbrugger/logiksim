#ifndef LOGICSIM_ALGORITHM_MERGED_FOR_EACH_H
#define LOGICSIM_ALGORITHM_MERGED_FOR_EACH_H

#include <iterator>
#include <ranges>

namespace logicsim {

template <std::input_iterator I1, std::sentinel_for<I1> S1, std::input_iterator I2,
          std::sentinel_for<I2> S2, typename Fun, class Comp = std::ranges::less>
auto merged_for_each(I1 first1, S1 last1, I2 first2, S2 last2, Fun f, Comp comp = {})
    -> void {
    while (first1 != last1 && first2 != last2) {
        std::invoke(f, *first1, *first2);

        if (std::invoke(comp, *first1, *first2)) {
            ++first1;
        } else {
            ++first2;
        }
    }
}

template <std::ranges::input_range R1, std::ranges::input_range R2, typename Fun,
          class Comp = std::ranges::less>
auto merged_for_each(R1&& r1, R2&& r2, Fun f, Comp comp = {}) -> void {
    merged_for_each(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2),
                    std::ranges::end(r2), std::move(f), std::move(comp));
}

}  // namespace logicsim

#endif
