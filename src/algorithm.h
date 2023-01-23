#ifndef LOGIKSIM_ALGORITHMS_H
#define LOGIKSIM_ALGORITHMS_H

#include <algorithm>
#include <concepts>
#include <exception>
#include <functional>
#include <iostream>
#include <iterator>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
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

//
// has duplicates
//

struct always_false {
    [[nodiscard]] constexpr auto operator()(auto&& left [[maybe_unused]], auto&& right
                                            [[maybe_unused]]) const noexcept -> bool {
        return false;
    }
};

/// good for small sizes, scales with O(n^2)
template <class Proj = std::identity, class Comp = std::equal_to<>,
          class Ignore = always_false>
auto has_duplicates_quadratic(std::bidirectional_iterator auto begin,
                              std::bidirectional_iterator auto end, Proj proj = {},
                              Comp comp = {}, Ignore ignore = {}) -> bool {
    if (begin == end) {
        return false;
    }
    for (auto i1 = begin; i1 != std::prev(end); ++i1) {
        for (auto i2 = std::next(i1); i2 != end; ++i2) {
            if (ignore(i1, i2)) {
                continue;
            }
            if (comp(std::invoke(proj, *i1), std::invoke(proj, *i2))) {
                return true;
            }
        }
    }
    return false;
}

/// good for small sizes, scales with O(n^2)
template <class Comp>
auto has_duplicates_quadratic_custom(std::bidirectional_iterator auto begin,
                                     std::bidirectional_iterator auto end, Comp comp)
    -> bool {
    if (begin == end) {
        return false;
    }
    for (auto i1 = begin; i1 != std::prev(end); ++i1) {
        for (auto i2 = std::next(i1); i2 != end; ++i2) {
            if (comp(i1, i2)) {
                return true;
            }
        }
    }
    return false;
}

bool has_duplicates_quadratic(const std::ranges::input_range auto& range) {
    if (std::size(range) <= 1) {
        return false;
    }

    // TODO refactor using above method
    for (auto i1 = std::begin(range); i1 != std::end(range) - 1; ++i1) {
        for (auto i2 = i1 + 1; i2 != std::end(range); ++i2) {
            if (*i1 == *i2) {
                return true;
            }
        }
    }
    return false;
}

//
// all_equal
//

auto all_equal(std::input_iterator auto first, std::input_iterator auto last, auto value)
    -> bool {
    return std::all_of(first, last, [&](auto& item) { return item == value; });
}

auto all_equal(std::ranges::input_range auto&& range, auto value) -> bool {
    return all_equal(std::ranges::begin(range), std::ranges::begin(range), value);
}

//
// distance_fast
//

template <typename IteratorFirst, typename IteratorLast>
    requires std::sized_sentinel_for<IteratorLast, IteratorFirst>
auto distance_fast(IteratorFirst first, IteratorLast last) {
    return last - first;
}

auto distance_fast(std::ranges::input_range auto&& range) {
    return distance_fast(std::begin(range), std::end(range));
}

//
// transform if
//

template <class IteratorFirst, class IteratorLast, class OutputIterator, class Pred,
          class Proj>
auto transform_if(IteratorFirst first1, IteratorLast last1, OutputIterator result,
                  Proj proj, Pred pred) -> OutputIterator {
    while (first1 != last1) {
        if (pred(*first1)) {
            *result = proj(*first1);
            ++result;
        }
        ++first1;
    }
    return result;
}

template <std::ranges::input_range R, class OutputIterator, class Pred, class Proj>
auto transform_if(R&& r, OutputIterator result, Proj proj, Pred pred) -> void {
    transform_if(std::begin(r), std::end(r), result, proj, pred);
}

//
// transform_to_vector
//

template <std::input_iterator InputIt, class Function>
    requires std::sized_sentinel_for<InputIt, InputIt>
constexpr auto transform_to_vector(InputIt first, InputIt last, Function func) {
    using result_type = std::decay_t<decltype(std::invoke(func, *first))>;

    std::vector<result_type> result {};
    result.reserve(distance_fast(first, last));

    std::transform(first, last, std::back_inserter(result), [func](auto&& item) {
        return std::invoke(func, std::forward<decltype(item)>(item));
    });
    return result;
}

template <class Function>
constexpr auto transform_to_vector(std::ranges::input_range auto&& range, Function func) {
    return transform_to_vector(std::ranges::begin(range), std::ranges::end(range), func);
}

// ransform_to_container

template <class Container, std::input_iterator InputIt, class Function>
    requires std::sized_sentinel_for<InputIt, InputIt>
constexpr auto transform_to_container(InputIt first, InputIt last, Function func)
    -> Container {
    Container result {};
    result.reserve(distance_fast(first, last));

    std::transform(first, last, std::back_inserter(result), [func](auto&& item) {
        return std::invoke(func, std::forward<decltype(item)>(item));
    });
    return result;
}

template <class Container, class Function>
constexpr auto transform_to_container(std::ranges::input_range auto&& range,
                                      Function func) -> Container {
    return transform_to_container<Container>(std::ranges::begin(range),
                                             std::ranges::end(range), func);
}

//
// sorted
//

template <class T>
constexpr auto sorted_ref(T& a, T& b) noexcept(noexcept(a <= b)) -> std::tuple<T&, T&> {
    if (a <= b) {
        return std::tie(a, b);
    }
    return std::tie(b, a);
}

template <class T>
constexpr auto sorted(T&& a, T&& b) noexcept(noexcept(a <= b))
    -> std::tuple<std::remove_cvref_t<T>, std::remove_cvref_t<T>> {
    if (a <= b) {
        return std::tie(std::forward<T>(a), std::forward<T>(b));
    }
    return std::tie(std::forward<T>(b), std::forward<T>(a));
}

}  // namespace logicsim

#endif
