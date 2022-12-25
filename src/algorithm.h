#ifndef LOGIKSIM_ALGORITHMS_H
#define LOGIKSIM_ALGORITHMS_H

#include <boost/container/small_vector.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <range/v3/all.hpp>

#include <algorithm>
#include <concepts>
#include <exception>
#include <iostream>
#include <iterator>
#include <vector>

namespace logicsim {

template <class T, typename ApplyFunc, typename WhileFunc>
    requires requires(T q, ApplyFunc apply_func, WhileFunc while_func,
                      const typename T::value_type& v) {
                 typename T::value_type;

                 { q.empty() } -> std::convertible_to<bool>;
                 { q.top() } -> std::same_as<const typename T::value_type&>;
                 q.pop();

                 apply_func(v);
                 { while_func(v) } -> std::convertible_to<bool>;
             }
void pop_while(T& queue, ApplyFunc apply_func, WhileFunc while_func) {
    while (!queue.empty()) {
        const auto& top = queue.top();
        if (!while_func(top)) {
            break;
        }
        apply_func(top);
        queue.pop();
    }
}

/// good for small ranges, scales with O(n^2)
bool has_duplicates_quadratic(const ranges::input_range auto&& range) {
    if (std::size(range) <= 1) {
        return false;
    }

    // TODO refactor using ranges
    for (auto i1 = std::begin(range); i1 != std::end(range) - 1; ++i1) {
        for (auto i2 = i1 + 1; i2 != std::end(range); ++i2) {
            if (*i1 == *i2) {
                return true;
            }
        }
    }
    return false;
}

auto all_equal(std::input_iterator auto first, std::input_iterator auto last, auto value)
    -> bool {
    return std::all_of(first, last, [&](auto& item) { return item == value; });
}

auto all_equal(std::ranges::input_range auto&& range, auto value) -> bool {
    return all_equal(std::ranges::begin(range), std::ranges::begin(range), value);
}

template <typename IteratorFirst, typename IteratorLast>
    requires std::sized_sentinel_for<IteratorLast, IteratorFirst>
auto distance_fast(IteratorFirst first, IteratorLast last) {
    return last - first;
}

template <std::input_iterator InputIt, class Function>
auto transform_to_vector(InputIt first, InputIt last, Function func) {
    using result_type = std::invoke_result_t<Function, typename InputIt::value_type>;

    std::vector<result_type> result;
    result.reserve(distance_fast(first, last));

    std::transform(first, last, std::back_inserter(result), func);
    return result;
}

}  // namespace logicsim

#endif
