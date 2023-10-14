#ifndef LOGICSIM_ITERATOR_ADAPTOR_ENUMERATE_H
#define LOGICSIM_ITERATOR_ADAPTOR_ENUMERATE_H

#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>  // std::pair

namespace logicsim {

namespace detail {

template <std::input_iterator I, std::sentinel_for<I> S, typename C>
struct enumerate_sentinel;

template <std::input_iterator I, typename C>
struct enumerate_iterator {
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;

    using counter_value_type = std::remove_cvref_t<C>;
    using difference_type = std::iter_difference_t<counter_value_type>;

    using value_type = std::pair<counter_value_type, std::iter_reference_t<I>>;
    using reference = value_type;
    using pointer = void;

    I iterator {};
    counter_value_type counter {0};

    auto operator++() -> enumerate_iterator& {
        ++counter;
        ++iterator;
        return *this;
    }

    auto operator++(int) -> enumerate_iterator {
        auto tmp = *this;
        operator++();
        return tmp;
    }

    auto operator*() const -> reference {
        return reference {counter, *iterator};
    }

    auto operator==(const enumerate_iterator& other) const -> bool {
        return iterator == other.iterator;
    }
};

template <std::input_iterator I, std::sentinel_for<I> S, typename C>
struct enumerate_sentinel {
    S sentinel {};

    friend auto operator==(const enumerate_iterator<I, C>& it,
                           const enumerate_sentinel<I, S, C>& s) -> bool {
        return it.iterator == s.sentinel;
    }

    friend auto operator==(const enumerate_sentinel<I, S, C>& s,
                           const enumerate_iterator<I, C>& it) -> bool {
        return it.iterator == s.sentinel;
    }
};

template <std::input_iterator I, std::sentinel_for<I> S, typename C>
struct enumerate_view {
    I first {};
    S last {};

    using iterator = enumerate_iterator<I, C>;
    using sentinel = enumerate_sentinel<I, S, C>;

    using value_type = typename iterator::value_type;

    auto begin() const {
        return iterator {first};
    }

    auto end() const {
        return sentinel {last};
    }

    static_assert(std::forward_iterator<iterator>);
    static_assert(std::sentinel_for<sentinel, iterator>);
};

template <std::ranges::input_range R, typename C>
struct enumerate_range {
    using I = std::ranges::iterator_t<R>;
    using S = std::ranges::sentinel_t<R>;

    using iterator = enumerate_iterator<I, C>;
    using sentinel = enumerate_sentinel<I, S, C>;

    using value_type = typename iterator::value_type;

    R range;

    auto begin() {
        return iterator {std::ranges::begin(range)};
    }

    auto end() {
        return sentinel {std::ranges::end(range)};
    }

    auto begin() const {
        using I_ = decltype(std::ranges::begin(range));

        using const_iterator_ = enumerate_iterator<I_, C>;
        static_assert(std::forward_iterator<const_iterator_>);

        return const_iterator_ {std::ranges::begin(range)};
    }

    auto end() const {
        using I_ = decltype(std::ranges::begin(range));
        using S_ = decltype(std::ranges::end(range));

        using const_iterator_ = enumerate_iterator<I_, C>;
        using const_sentinel_ = enumerate_sentinel<I_, S_, C>;
        static_assert(std::sentinel_for<const_sentinel_, const_iterator_>);

        return const_sentinel_ {std::ranges::end(range)};
    }

    static_assert(std::forward_iterator<iterator>);
    static_assert(std::sentinel_for<sentinel, iterator>);
};

template <typename C>
concept enumerate_counter = std::incrementable<std::remove_cvref_t<C>>;

}  // namespace detail

/**
 * @brief Returns non-owning enumerated view of the iterator pairs.
 *
 * For each value in the iterator a pair is returned with the counter index
 * and a reference to the value.
 *
 * Note that enumerate supports custom counter types.
 */
template <typename C = std::size_t, std::input_iterator I, std::sentinel_for<I> S>
    requires detail::enumerate_counter<C>
constexpr auto enumerate(I first, S last) {
    using view_t = detail::enumerate_view<I, S, C>;
    static_assert(std::ranges::forward_range<view_t>);

    return view_t {first, last};
}

/**
 * @brief Returns non-owning enumerated view of the range.
 *
 * For each value in the iterator a pair is returned with the counter index
 * and a reference to the value.
 *
 * Note that enumerate supports custom counter types.
 */
template <typename C = std::size_t, std::ranges::input_range R>
    requires detail::enumerate_counter<C>
constexpr auto enumerate(R& r) {
    return enumerate<C>(std::ranges::begin(r), std::ranges::end(r));
}

/**
 * @brief Returns owning enumerated range.
 *
 * For each value in the iterator a pair is returned with the counter index
 * and a reference to the value.
 *
 * Note that enumerate supports custom counter types.
 */
template <typename C = std::size_t, std::ranges::input_range R>
    requires detail::enumerate_counter<C>
constexpr auto enumerate(R&& r) {
    static_assert(!std::is_lvalue_reference_v<R>, "bad call");

    using range_t = detail::enumerate_range<R, C>;
    static_assert(std::ranges::forward_range<range_t>);

    return range_t {std::move(r)};
}

}  // namespace logicsim

template <std::input_iterator I, std::sentinel_for<I> S, typename C>
inline constexpr bool
    std::ranges::enable_view<logicsim::detail::enumerate_view<I, S, C>> = true;

template <std::input_iterator I, std::sentinel_for<I> S, typename C>
inline constexpr bool
    std::ranges::enable_borrowed_range<logicsim::detail::enumerate_view<I, S, C>> = true;

#endif
