#ifndef LOGICSIM_ALGORITHM_POP_WHILE_H
#define LOGICSIM_ALGORITHM_POP_WHILE_H

#include <concepts>
#include <functional>

namespace logicsim {

/**
 * brief: Remove items from the queue while functions is true.
 */
template <class T, typename ApplyFunc, typename WhileFunc>
    requires requires(T q, ApplyFunc apply_func, WhileFunc while_func,
                      const typename T::value_type& v) {
        typename T::value_type;

        { q.empty() } -> std::convertible_to<bool>;
        { q.top() } -> std::same_as<const typename T::value_type&>;
        q.pop();

        std::invoke(apply_func, v);
        { std::invoke(while_func, v) } -> std::convertible_to<bool>;
    }
void pop_while(T& queue, ApplyFunc apply_func, WhileFunc while_func) {
    while (!queue.empty()) {
        const auto& top = queue.top();
        if (!std::invoke(while_func, top)) {
            break;
        }
        std::invoke(apply_func, top);
        queue.pop();
    }
}

}  // namespace logicsim

#endif
