#ifndef LOGIKSIM_ALGORITHMS_H
#define LOGIKSIM_ALGORITHMS_H

#include <concepts>

namespace logicsim {

    template <class T, typename ApplyFunc, typename WhileFunc>
    requires requires (T q, ApplyFunc apply_func, WhileFunc while_func, const typename T::value_type& v) {
        typename T::value_type;

        { q.empty() } -> std::convertible_to<bool>;
        { q.top() } -> std::same_as<const typename T::value_type&>;
        q.pop();

        apply_func(v);
        { while_func(v) } -> std::convertible_to<bool>;
    }

    void queue_apply_while(T &queue, ApplyFunc apply_func, WhileFunc while_func) {
        while (!queue.empty()) {
            const auto &top = queue.top();
            if (!while_func(top)) {
                break;
            }
            apply_func(top);
            queue.pop();
        }
    }

}

#endif
