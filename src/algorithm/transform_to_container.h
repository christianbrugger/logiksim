#ifndef LOGICSIM_ALGORITHM_TRANSFORM_TO_CONTAINER_H
#define LOGICSIM_ALGORITHM_TRANSFORM_TO_CONTAINER_H

#include "algorithm/distance_fast.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>
#include <utility>

namespace logicsim {

/**
 * brief: Stores the transformed values of the range to a new container.
 */
template <class Container, std::input_iterator InputIt,
          std::sized_sentinel_for<InputIt> Sentinel, class Function>
constexpr auto transform_to_container(InputIt first, Sentinel last,
                                      Function func) -> Container {
    Container result {};
    result.reserve(static_cast<std::size_t>(distance_fast(first, last)));

    std::ranges::transform(first, last, std::back_inserter(result), [func](auto&& item) {
        return std::invoke(func, std::forward<decltype(item)>(item));
    });
    return result;
}

/**
 * brief: Stores the transformed values of the range to a new container.
 */
template <class Container, class Function>
constexpr auto transform_to_container(std::ranges::input_range auto&& range,
                                      Function func) -> Container {
    return transform_to_container<Container>(std::ranges::begin(range),
                                             std::ranges::end(range), func);
}

}  // namespace logicsim

#endif
