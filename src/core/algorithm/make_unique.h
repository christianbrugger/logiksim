#ifndef LOGICSIM_ALGORITHM_MAKE_UNIQUE_H
#define LOGICSIM_ALGORITHM_MAKE_UNIQUE_H

#include <algorithm>
#include <ranges>
#include <vector>

namespace logicsim {

template <class Container, class Comp = std::ranges::less>
auto sort_and_make_unique(Container& data, Comp comp_ = {}) -> void {
    std::ranges::sort(data, std::move(comp_));
    const auto erasable = std::ranges::unique(data);
    data.erase(erasable.begin(), erasable.end());
}

}  // namespace logicsim

#endif
