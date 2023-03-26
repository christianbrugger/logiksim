#ifndef LOGIKSIM_ALGORITHMS_H
#define LOGIKSIM_ALGORITHMS_H

#include "iterator_adaptor.h"

#include <gsl/gsl>

#include <algorithm>
#include <cassert>
#include <cfenv>
#include <concepts>
#include <exception>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
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
// contains (from C++23 algorithm)
//

template <std::input_iterator I, std::sentinel_for<I> S, class T,
          class Proj = std::identity>
    requires std::indirect_binary_predicate<std::ranges::equal_to,
                                            std::projected<I, Proj>, const T*>
constexpr bool contains(I first, S last, const T& value, Proj proj = {}) {
    return std::ranges::find(first, last, value, proj) != last;
}

template <std::ranges::input_range R, class T, class Proj = std::identity>
    requires std::indirect_binary_predicate<
        std::ranges::equal_to, std::projected<std::ranges::iterator_t<R>, Proj>, const T*>
constexpr bool contains(R&& r, const T& value, Proj proj = {}) {
    return contains(std::ranges::begin(r), std::ranges::end(r), value, std::move(proj));
}

//
// copy adjacent if
//

template <
    std::forward_iterator I, std::sentinel_for<I> S, std::weakly_incrementable O,
    class Proj = std::identity,
    std::indirect_binary_predicate<std::projected<I, Proj>, std::projected<I, Proj>> Pred
    = std::ranges::equal_to>
    requires std::indirectly_writable<O, std::indirect_result_t<Proj&, I>>
constexpr auto copy_adjacent_if(I first, S last, O result, Pred pred = {}, Proj proj = {})
    -> void {
    auto&& it = first;

    while (it != last) {
        it = std::ranges::adjacent_find(it, last, pred, proj);

        if (it != last) {
            result = std::invoke(proj, *it);
            ++result;
            ++it;
        }
    }
}

//
// accumulate
//

template <std::input_iterator I, class T>
constexpr T accumulate(I first, I last, T init) {
    return std::accumulate(first, last, std::move(init));
}

template <std::ranges::input_range R, class T>
constexpr T accumulate(R&& r, T init) {
    return std::accumulate(std::ranges::begin(r), std::ranges::end(r), std::move(init));
}

//
// Variant overload
//

template <typename... Func>
struct overload : Func... {
    using Func::operator()...;
};

template <class... Ts>
overload(Ts...) -> overload<Ts...>;

//
// Round to
//

inline auto round_fast(double value) -> double {
    // std::nearbyint is faster than std::round, but we need to check rounding mode
    assert(std::fegetround() == FE_TONEAREST);
    return std::nearbyint(value);
}

template <typename result_type = double>
auto round_to(double value) -> result_type {
    return gsl::narrow<result_type>(round_fast(value));
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
template <class Proj = std::identity, class Comp = std::equal_to<>,
          class Ignore = always_false>
bool has_duplicates_quadratic(std::ranges::input_range auto&& range, Proj proj = {},
                              Comp comp = {}, Ignore ignore = {}) {
    return has_duplicates_quadratic(std::ranges::begin(range), std::ranges::end(range),
                                    std::move(proj), std::move(comp), std::move(ignore));
}

/// good for small sizes, scales with O(n^2)
//
// uses compare with iterators, instead of values
template <class Comp>
auto has_duplicates_quadratic_iterator(std::bidirectional_iterator auto begin,
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
// adjacent count if
//
// std::indirect_unary_predicate<std::projected<I, Proj>> Pred

template <
    std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
    std::indirect_binary_predicate<std::projected<I, Proj>, std::projected<I, Proj>> Pred
    = std::ranges::equal_to>
constexpr auto adjacent_count_if(I first, S last, Pred pred = {}, Proj proj = {})
    -> std::iter_difference_t<I> {
    static_assert(std::input_iterator<I>);
    static_assert(std::sentinel_for<I, S>);
    static_assert(std::indirect_binary_predicate<Pred, std::projected<I, Proj>,
                                                 std::projected<I, Proj>>);

    auto count = std::iter_difference_t<I> {0};
    if (first == last) {
        return count;
    }

    auto next = first;
    ++next;
    for (; next != last; ++next, ++first) {
        if (std::invoke(pred, std::invoke(proj, *first), std::invoke(proj, *next))) {
            ++count;
        }
    }
    return count;
}

template <std::ranges::forward_range R, class Proj = std::identity,
          std::indirect_binary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>,
                                         std::projected<std::ranges::iterator_t<R>, Proj>>
              Pred
          = std::ranges::equal_to>
constexpr auto adjacent_count_if(R&& r, Pred pred = {}, Proj proj = {}) {
    return adjacent_count_if(std::ranges::begin(r), std::ranges::end(r), std::ref(pred),
                             std::ref(proj));
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
            *result = std::invoke(proj, *first1);
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
// transform_combine_while
//

template <class IteratorFirst, class IteratorLast, class OutputIterator, class MakeState,
          class Pred, class Update = std::identity, class Proj = std::identity>
auto transform_combine_while(IteratorFirst first, IteratorLast last,
                             OutputIterator result, MakeState make_state, Pred predicate,
                             Update update, Proj project) -> OutputIterator {
    auto i0 = first;

    while (i0 != last) {
        auto i1 = i0 + 1;
        auto state = make_state(i0);

        while (i1 != last && predicate(state, i1)) {
            state = update(state, i1);
            ++i1;
        }

        *result = project(state);
        ++result;

        i0 = i1;
    }

    return result;
}

template <std::ranges::input_range R, class OutputIterator, class MakeState, class Pred,
          class Update = std::identity, class Proj = std::identity>
auto transform_combine_while(R&& r, OutputIterator result, MakeState make_state,
                             Pred predicate, Update update = {}, Proj project = {})
    -> void {
    transform_combine_while(std::begin(r), std::end(r), result, make_state, predicate,
                            update, project);
}

//
// depth first visitor
//
//  start_node:
//      The index for which the algorithm should start the search.
//
//  visited_state:
//      Datstructure to store the visited state. Should be initialized to false:
//          visited_state[IndexType] = true;
//          visited_state[IndexType] -> bool;
//      Can be a vector for a continous integer index or in general an associative map.
//
//  discover_connection:
//      From the given node add all connected nodes to the output iterator as IndexType.
//
//      auto discover_connection(IndexType node,
//                               std::output_iterator<IndexType> auto result) -> void;
//
//  visit_edge:
//      Called for each edge with signature:
//
//      auto visit_edge(IndexType a, IndexType b) -> void;
//
//  Resuts: returns true if it finds a loop.
//
template <typename VisitedStore, typename DiscoverConnected, typename EdgeVisitor,
          typename IndexType>
auto depth_first_visitor(IndexType start_node, VisitedStore& visited,
                         DiscoverConnected discover_connections, EdgeVisitor visit_edge)
    -> bool {
    std::vector<std::pair<IndexType, IndexType>> edges_stack {};

    const auto result = transform_output_iterator(
        [=](IndexType second) { return std::make_pair(start_node, second); },
        std::back_inserter(edges_stack));
    discover_connections(start_node, result);

    while (true) {
        if (edges_stack.empty()) {
            return false;
        }
        const auto edge = edges_stack.back();
        edges_stack.pop_back();

        if (visited[edge.second]) {
            return true;
        }
        visited[edge.second] = true;

        visit_edge(edge.first, edge.second);
        discover_connections(
            edge.second,
            transform_if_output_iterator(
                [=](IndexType second) { return second != edge.first; },
                [=](IndexType second) { return std::make_pair(edge.second, second); },
                std::back_inserter(edges_stack)));
    }
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
        return std::make_tuple(std::forward<T>(a), std::forward<T>(b));
    }
    return std::make_tuple(std::forward<T>(b), std::forward<T>(a));
}

}  // namespace logicsim

#endif
