#ifndef LOGIKSIM_ALGORITHMS_H
#define LOGIKSIM_ALGORITHMS_H

#include <boost/container/small_vector.hpp>

#include <concepts>
#include <format>
#include <ranges>
#include <iostream>
#include <format>


namespace logicsim {

    [[noreturn]] void throw_exception(const char* msg);


    template <class T, typename ApplyFunc, typename WhileFunc>
    requires requires (T q, ApplyFunc apply_func, WhileFunc while_func, const typename T::value_type& v) {
        typename T::value_type;

        { q.empty() } -> std::convertible_to<bool>;
        { q.top() } -> std::same_as<const typename T::value_type&>;
        q.pop();

        apply_func(v);
        { while_func(v) } -> std::convertible_to<bool>;
    }

    void pop_while(T &queue, ApplyFunc apply_func, WhileFunc while_func) {
        while (!queue.empty()) {
            const auto &top = queue.top();
            if (!while_func(top)) {
                break;
            }
            apply_func(top);
            queue.pop();
        }
    }

    // good for small ranges, scales with O(n^2)
    bool has_duplicates_quadratic(const std::ranges::input_range auto&& range) {
        if (std::size(range) <= 1)
            return false;

        for (auto i1 = std::begin(range); i1 != std::end(range) - 1; ++i1)
            for (auto i2 = std::begin(range) + 1; i2 != std::end(range); ++i2)
                if (*i1 == *i2)
                    return true;
        return false;
    }
}


template<typename T, std::size_t N, class CharT>
struct std::formatter<boost::container::small_vector<T, N>, CharT> : std::formatter<string_view> {
    auto format(const boost::container::small_vector<T, N>& obj, std::format_context& ctx) {
        std::string temp;

        for (const auto& elem : obj)
            std::format_to(std::back_inserter(temp), "{}, ", elem);

        return std::formatter<string_view>::format(temp, ctx);
    }
};

#endif
