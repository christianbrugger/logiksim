#ifndef LOGICSIM_ALGORITHM_COMPARE_SORTED_H
#define LOGICSIM_ALGORITHM_COMPARE_SORTED_H

#include <algorithm>
#include <ranges>

namespace logicsim {

/**
 * brief: Sorts the two given ranges and compares them.
 */
template <std::ranges::sized_range R1, std::ranges::sized_range R2>
constexpr auto compare_sorted(R1&& r1, R2&& r2) {
    if (std::ranges::size(r1) != std::ranges::size(r2)) {
        return false;
    }

    std::ranges::sort(r1);
    std::ranges::sort(r2);

    return r1 == r2;
}

}  // namespace logicsim

#endif
