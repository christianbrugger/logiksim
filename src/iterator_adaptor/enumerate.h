#ifndef LOGICSIM_ITERATOR_ADAPTOR_ENUMERATE_H
#define LOGICSIM_ITERATOR_ADAPTOR_ENUMERATE_H

#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>  // std::pair, std::forward

namespace logicsim {

namespace detail {

template <std::input_iterator I, std::sentinel_for<I> S>
struct enumerate_sentinel;

template <std::input_iterator I>
struct enumerate_iterator {
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;

    using difference_type = std::ptrdiff_t;
    // using value_type = std::pair<int, int>;
    // using reference = std::pair<int, int>;
    // using pointer = void;

    // using value_type = std::pair<int, typename std::iter_value_t<I>>;
    // using reference = std::pair<int&, typename std::iter_reference_t<I>>;

    using value_type = std::pair<int, std::iter_value_t<I>>;
    using reference = value_type;
    using pointer = void;

    // using difference_type = std::ranges::range_difference_t<T>;
    // using value_type = std::pair<difference_type, std::ranges::range_value_t<T>>;
    // using reference = std::pair<difference_type, std::ranges::range_reference_t<T>>;
    // using pointer = void;

    I iterator {};
    difference_type counter {};

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
        return reference(counter, *iterator);
    }

    auto operator==(const enumerate_iterator& other) const -> bool {
        return iterator == other.iterator;
    }
};

template <std::input_iterator I, std::sentinel_for<I> S>
struct enumerate_sentinel {
    S sentinel {};

    friend auto operator==(const enumerate_iterator<I>& it,
                           const enumerate_sentinel<I, S>& s) -> bool {
        return it.iterator == s.sentinel;
    }

    friend auto operator==(const enumerate_sentinel<I, S>& s,
                           const enumerate_iterator<I>& it) -> bool {
        return it.iterator == s.sentinel;
    }
};

template <std::input_iterator I, std::sentinel_for<I> S>
struct enumerate_view {
    I first {};
    S last {};

    using iterator = enumerate_iterator<I>;
    using sentinel = enumerate_sentinel<I, S>;

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

template <std::ranges::input_range R>
struct enumerate_range {
    using I = std::ranges::iterator_t<R>;
    using S = std::ranges::sentinel_t<R>;

    using iterator = enumerate_iterator<I>;
    using sentinel = enumerate_sentinel<I, S>;

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

        using const_iterator_ = enumerate_iterator<I_>;
        static_assert(std::forward_iterator<const_iterator_>);

        return const_iterator_ {std::ranges::begin(range)};
    }

    auto end() const {
        using I_ = decltype(std::ranges::begin(range));
        using S_ = decltype(std::ranges::end(range));

        using const_iterator_ = enumerate_iterator<I_>;
        using const_sentinel_ = enumerate_sentinel<I_, S_>;
        static_assert(std::sentinel_for<const_sentinel_, const_iterator_>);

        return const_sentinel_ {std::ranges::end(range)};
    }

    static_assert(std::forward_iterator<iterator>);
    static_assert(std::sentinel_for<sentinel, iterator>);
};

}  // namespace detail

template <std::input_iterator I, std::sentinel_for<I> S>
constexpr auto enumerate(I first, S last) {
    using view_t = detail::enumerate_view<I, S>;
    static_assert(std::ranges::forward_range<view_t>);

    return view_t {first, last};
}

template <std::ranges::input_range R>
constexpr auto enumerate(R& r) {
    return enumerate(std::ranges::begin(r), std::ranges::end(r));
}

template <std::ranges::input_range R>
constexpr auto enumerate(R&& r) {
    static_assert(!std::is_lvalue_reference_v<R>, "bad call");

    using range_t = detail::enumerate_range<R>;
    static_assert(std::ranges::forward_range<range_t>);

    return range_t {std::move(r)};
}

}  // namespace logicsim

#endif
