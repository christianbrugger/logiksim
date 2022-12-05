#ifndef LOGIKSIM_ALGORITHMS_H
#define LOGIKSIM_ALGORITHMS_H

#include <boost/container/small_vector.hpp>
#include <range/v3/all.hpp>

#include <concepts>
#include <format>
#include <iostream>

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
    if (std::size(range) <= 1) return false;

    for (auto i1 = std::begin(range); i1 != std::end(range) - 1; ++i1)
        for (auto i2 = std::begin(range) + 1; i2 != std::end(range); ++i2)
            if (*i1 == *i2) return true;
    return false;
}

}  // namespace logicsim

#endif
