
#ifndef LOGIKSIM_RANGE_H
#define LOGIKSIM_RANGE_H

#include "concepts.h"

#include <fmt/core.h>

#include <concepts>
#include <iterator>
#include <ranges>
#include <type_traits>

namespace logicsim {

// Specialize this if the default is wrong.
template <class T>
inline constexpr T range_type_zero_value = T {0};

// define difference_type in your own type using this
template <class T>
using range_difference_t
    = std::conditional_t<std::is_integral_v<T>,
                         std::conditional_t<sizeof(T) < sizeof(int), int, long long>,
                         std::iter_difference_t<T>>;

namespace detail {

template <typename T>
    requires std::copyable<T>
struct range_iterator_t {
    T current_ {};

    using iterator_concept = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;
    using value_type = T;
    using difference_type = range_difference_t<T>;
    using pointer = T*;
    using reference = T&;

    [[nodiscard]] constexpr auto operator*() const
        noexcept(std::is_nothrow_copy_constructible_v<T>) -> T {
        return current_;
    }

    // Prefix increment
    constexpr auto operator++() noexcept(noexcept(++current_)) -> range_iterator_t& {
        ++current_;
        return *this;
    }

    // Postfix increment
    constexpr auto operator++(int) noexcept(noexcept(++current_)
                                            && std::is_nothrow_copy_constructible_v<T>)
        -> range_iterator_t {
        auto tmp = *this;
        ++current_;
        return tmp;
    }

    [[nodiscard]] friend constexpr auto operator==(
        const range_iterator_t<T>& left,
        const range_iterator_t<T>& right) noexcept(noexcept(left.current_
                                                            == right.current_)) -> bool {
        // this way we generate an empty range when last < first
        return left.current_ >= right.current_;
    }
};

template <typename T>
struct range_t {
   public:
    using value_type = T;
    using pointer = T*;
    using reference = T&;

    // static_assert(std::input_iterator<detail::range_iterator_t<T>>);
    // static_assert(
    //     std::sentinel_for<detail::range_sentinel_t<T>, detail::range_iterator_t<T>>);

    // clang-format off
    range_t() = default;

    // clang-format on

    [[nodiscard]] constexpr explicit range_t(T stop) noexcept(
        std::is_nothrow_move_constructible_v<T>)
        : stop_ {std::move(stop)} {}

    // TODO remove linter warning
    [[nodiscard]] constexpr explicit range_t(T start, T stop) noexcept(
        std::is_nothrow_move_constructible_v<T>)
        : start_ {std::move(start)}, stop_ {std::move(stop)} {}

    [[nodiscard]] constexpr auto begin() const
        noexcept(std::is_nothrow_copy_constructible_v<T>) -> range_iterator_t<T> {
        return range_iterator_t<T> {start_};
    }

    [[nodiscard]] constexpr auto end() const
        noexcept(std::is_nothrow_copy_constructible_v<T>) -> range_iterator_t<T> {
        return range_iterator_t<T> {stop_};
    }

    [[nodiscard]] constexpr auto size() const -> range_difference_t<T>
        requires explicitly_convertible_to<T, range_difference_t<T>>
    {
        using Dt = range_difference_t<T>;

        auto start = Dt {start_};
        auto stop = Dt {stop_};
        auto zero = Dt {zero_};

        return std::max(stop - start, zero);
    }

    [[nodiscard]] constexpr auto empty() const noexcept(noexcept(begin() == end()))
        -> bool {
        return begin() == end();
    }

    [[nodiscard]] constexpr auto format() const -> std::string {
        return fmt::format("range({}, {})", start_, stop_);
    }

   private:
    static constexpr auto zero_ = range_type_zero_value<T>;
    T start_ {zero_};
    T stop_ {zero_};
};
}  // namespace detail

template <typename T>
concept range_value_type
    = std::weakly_incrementable<T> && std::equality_comparable<T>
      && std::totally_ordered<T> && std::input_iterator<detail::range_iterator_t<T>>;

template <range_value_type T>
[[nodiscard]] constexpr auto range(T stop) noexcept(
    std::is_nothrow_constructible_v<detail::range_t<T>, T>) -> detail::range_t<T> {
    return detail::range_t<T> {stop};
}  // namespace logicsim

template <range_value_type T>
[[nodiscard]] constexpr auto range(T start, T stop) noexcept(
    std::is_nothrow_constructible_v<detail::range_t<T>, T, T>) -> detail::range_t<T> {
    return detail::range_t<T> {start, stop};
}

}  // namespace logicsim

template <typename T>
inline constexpr bool std::ranges::enable_view<logicsim::detail::range_t<T>> = true;

template <typename T>
inline constexpr bool std::ranges::enable_borrowed_range<logicsim::detail::range_t<T>>
    = true;

template <typename T>
struct fmt::formatter<logicsim::detail::range_t<T>> {
    constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

    auto format(const logicsim::detail::range_t<T>& obj, fmt::format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

#endif
