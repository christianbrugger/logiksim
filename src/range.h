
#ifndef TEST_RANGE_H
#define TEST_RANGE_H

#include <fmt/core.h>

#include <concepts>
#include <iterator>

namespace logicsim {

namespace detail {

template <class T>
using range_difference_t
    = std::conditional_t<std::is_integral_v<T> && sizeof(T) < sizeof(int), int,
                         long long>;

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
};

template <typename T>
struct range_sentinel_t {
    T last_ {};

    [[nodiscard]] friend constexpr auto operator==(
        const range_iterator_t<T>& left,
        const range_sentinel_t<T>& right) noexcept(noexcept(left.current_ == right.last_))
        -> bool {
        // this way we generate an empty range when last < first
        return left.current_ >= right.last_;
    }
};

template <typename T>
    requires std::equality_comparable<T>
struct range_t {
   public:
    using value_type = T;
    using pointer = T*;
    using reference = T&;

    static_assert(std::input_iterator<detail::range_iterator_t<T>>);
    static_assert(
        std::sentinel_for<detail::range_sentinel_t<T>, detail::range_iterator_t<T>>);

    // clang-format off
    range_t() requires std::default_initializable<T> = default;

    // clang-format on

    [[nodiscard]] constexpr explicit range_t(T stop) noexcept(
        std::is_nothrow_move_constructible_v<T>&&
            std::is_nothrow_default_constructible_v<T>)
        requires std::default_initializable<T>
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
        noexcept(std::is_nothrow_copy_constructible_v<T>) -> range_sentinel_t<T> {
        return range_sentinel_t<T> {stop_};
    }

    [[nodiscard]] constexpr auto size() const
        noexcept(noexcept(std::max(stop_ - start_, T {}))) -> T {
        return std::max(stop_ - start_, T {});
    }

    [[nodiscard]] constexpr auto empty() const noexcept(noexcept(start_ >= stop_))
        -> bool {
        return start_ >= stop_;
    }

    [[nodiscard]] constexpr auto format() const -> std::string {
        return fmt::format("range({}, {})", start_, stop_);
    }

   private:
    T start_ {};
    T stop_ {};
};
}  // namespace detail

template <typename T>
[[nodiscard]] constexpr auto range(T stop) noexcept(
    std::is_nothrow_constructible_v<detail::range_t<T>, T>) -> detail::range_t<T>
    requires std::default_initializable<T> && std::equality_comparable<T>
{
    return detail::range_t<T> {stop};
}

template <typename T>
[[nodiscard]] constexpr auto range(T start, T stop) noexcept(
    std::is_nothrow_constructible_v<detail::range_t<T>, T, T>) -> detail::range_t<T>
    requires std::equality_comparable<T>
{
    return detail::range_t<T> {start, stop};
}

}  // namespace logicsim

template <typename T>
struct fmt::formatter<logicsim::detail::range_t<T>> {
    constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

    auto format(const logicsim::detail::range_t<T>& obj, fmt::format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

#endif
