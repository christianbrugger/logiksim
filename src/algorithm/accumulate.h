#ifndef LOGICSIM_ALGORITHM_ACCUMULATE_H
#define LOGICSIM_ALGORITHM_ACCUMULATE_H

#include <functional>  // std::invoke, std::identity
#include <iterator>
#include <numeric>
#include <ranges>
#include <utility>

namespace logicsim {

//
// C++20 accumulate that supports projection and ranges
//

template <std::input_iterator I, class T>
constexpr T accumulate(I first, I last, T init) {
    return std::accumulate(first, last, std::move(init));
}

template <std::ranges::input_range R, class T>
constexpr T accumulate(R&& r, T init) {
    return std::accumulate(std::ranges::begin(r), std::ranges::end(r), std::move(init));
}

template <std::input_iterator I, class T, class Proj = std::identity>
constexpr T accumulate(I first, I last, T init, Proj proj) {
    for (; first != last; ++first) {
        init = std::move(init) + std::invoke(proj, *first);
    }
    return init;
}

template <std::ranges::input_range R, class T, class Proj = std::identity>
constexpr T accumulate(R&& r, T init, Proj proj) {
    return std::accumulate(std::ranges::begin(r), std::ranges::end(r), std::move(init),
                           std::move(proj));
}

}  // namespace logicsim

#endif
