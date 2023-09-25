
#ifndef LOGIKSIM_RANGE_H
#define LOGIKSIM_RANGE_H

#include "concept.h"
#include "exception.h"
#include "difference_type.h"

#include <fmt/core.h>
#include <gsl/gsl>

#include <concepts>
#include <cstdlib>
#include <iterator>
#include <ranges>
#include <type_traits>

namespace logicsim {

// Specialize this if the default is wrong.
template <class T>
inline constexpr T range_type_zero_value = T {0};
template <class T>
inline constexpr T range_type_one_value = T {1};
template <class T>
using range_difference_t = safe_difference_t<T>;

namespace detail {

//
// range_iterator_t
//

template <typename T, bool forward>
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
        if constexpr (forward) {
            return current_;
        } else {
            auto res = current_;
            --res;
            return res;
        }
    }

    // Prefix increment
    constexpr auto operator++() noexcept(noexcept(++current_)) -> range_iterator_t& {
        if constexpr (forward) {
            ++current_;
        } else {
            --current_;
        }
        return *this;
    }

    // Postfix increment
    constexpr auto operator++(int) noexcept(noexcept(++current_) &&
                                            std::is_nothrow_copy_constructible_v<T>)
        -> range_iterator_t {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    [[nodiscard]] friend constexpr auto operator==(
        const range_iterator_t<T, forward>& left,
        const range_iterator_t<T, forward>& right) noexcept(noexcept(left.current_ ==
                                                                     right.current_))
        -> bool {
        // this way we generate an empty range when last < first
        if constexpr (forward) {
            return left.current_ >= right.current_;
        } else {
            return left.current_ <= right.current_;
        }
    }

    [[nodiscard]] auto operator-(const range_iterator_t<T, forward>& right) const
        noexcept(noexcept(this->current_ - right.current_)) -> difference_type
        requires requires(T v_) { v_ - v_; }
    {
        if constexpr (forward) {
            return this->current_ - right.current_;
        }
        return right.current_ - this->current_;
    }
};

template <typename T, bool forward>
struct range_t {
   public:
    using value_type = std::remove_cvref_t<T>;
    using pointer = value_type*;
    using reference = value_type&;

    range_t() = default;

    [[nodiscard]] constexpr explicit range_t(value_type stop) noexcept(
        std::is_nothrow_move_constructible_v<value_type>)
        : stop_ {std::move(stop)} {}

    // TODO remove linter warning
    [[nodiscard]] constexpr explicit range_t(value_type start, value_type stop) noexcept(
        std::is_nothrow_move_constructible_v<value_type>)
        : start_ {std::move(start)}, stop_ {std::move(stop)} {}

    [[nodiscard]] constexpr auto begin() const
        noexcept(std::is_nothrow_copy_constructible_v<value_type>)
            -> range_iterator_t<value_type, forward> {
        return range_iterator_t<value_type, forward> {start_};
    }

    [[nodiscard]] constexpr auto end() const
        noexcept(std::is_nothrow_copy_constructible_v<value_type>)
            -> range_iterator_t<value_type, forward> {
        return range_iterator_t<value_type, forward> {stop_};
    }

    [[nodiscard]] constexpr auto size() const -> range_difference_t<value_type>
        requires explicitly_convertible_to<value_type, range_difference_t<value_type>>
    {
        using Dt = range_difference_t<value_type>;

        Dt start;
        Dt stop;
        Dt zero;

        if constexpr (std::is_arithmetic_v<value_type>) {
            start = gsl::narrow<Dt>(start_);
            stop = gsl::narrow<Dt>(stop_);
            zero = gsl::narrow<Dt>(zero_);
        } else {
            start = Dt {start_};
            stop = Dt {stop_};
            zero = Dt {zero_};
        }

        if constexpr (forward) {
            return std::max(stop - start, zero);
        } else {
            return std::max(start - stop, zero);
        }
    }

    [[nodiscard]] constexpr auto empty() const noexcept(noexcept(begin() == end()))
        -> bool {
        return begin() == end();
    }

    [[nodiscard]] constexpr auto format() const -> std::string {
        if constexpr (forward) {
            return fmt::format("range({}, {})", start_, stop_);
        } else {
            return fmt::format("reverse_range({}, {})", stop_, start_);
        }
    }

    [[nodiscard]] constexpr auto reverse() const -> range_t<T, !forward> {
        return range_t<T, !forward> {stop_, start_};
    }

   private:
    static constexpr auto zero_ = range_type_zero_value<T>;
    value_type start_ {zero_};
    value_type stop_ {zero_};
};

//
// range_iterator_step_t
//

// calculate the size of a range
template <typename T>
auto range_step_size(T start_, T stop_, T step_) -> range_difference_t<T> {
    using Dt = range_difference_t<T>;

    auto start = Dt {start_};
    auto stop = Dt {stop_};
    auto step = Dt {step_};

    auto step_value = (step >= Dt {0}) ? step : Dt {-1} * step;
    auto difference = (step >= Dt {0}) ? (stop - start) : Dt {-1} * (stop - start);

    if (difference < Dt {0}) {
        return Dt {0};
    }

    auto a = difference / step_value;
    auto b = difference % step_value;

    if (b != Dt {0}) {
        return a + Dt {1};
    }
    return a;
}

template <typename T>
    requires std::copyable<T>
struct range_iterator_step_t {
    T current_ {};
    T step_ {};

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
    constexpr auto operator++() noexcept(noexcept(++current_)) -> range_iterator_step_t& {
        current_ += step_;
        return *this;
    }

    // Postfix increment
    constexpr auto operator++(int) noexcept(noexcept(++current_) &&
                                            std::is_nothrow_copy_constructible_v<T>)
        -> range_iterator_step_t {
        auto tmp = *this;
        current_ += step_;
        return tmp;
    }

    [[nodiscard]] friend constexpr auto operator==(const range_iterator_step_t<T>& left,
                                                   const range_iterator_step_t<T>& right)
        -> bool {
        // this way we generate an empty range when last < first
        return (left.step_ > T {0}) ? left.current_ >= right.current_
                                    : left.current_ <= right.current_;
    }

    [[nodiscard]] auto operator-(const range_iterator_step_t<T>& right) const
        -> difference_type {
        return range_step_size(this->current_, right.current_, this->step_);
    }
};

template <typename T>
struct range_step_t {
   public:
    using value_type = std::remove_cvref_t<T>;
    using pointer = value_type*;
    using reference = value_type&;

    range_step_t() = default;

    [[nodiscard]] constexpr explicit range_step_t(value_type start, value_type stop,
                                                  value_type step)
        : start_ {std::move(start)}, stop_ {std::move(stop)}, step_ {std::move(step)} {
        if (step_ == zero_) {
            throw_exception("Step cannot be zero.");
        }
    }

    [[nodiscard]] constexpr auto begin() const
        noexcept(std::is_nothrow_copy_constructible_v<value_type>)
            -> range_iterator_step_t<value_type> {
        return range_iterator_step_t<value_type> {start_, step_};
    }

    [[nodiscard]] constexpr auto end() const
        noexcept(std::is_nothrow_copy_constructible_v<value_type>)
            -> range_iterator_step_t<value_type> {
        return range_iterator_step_t<value_type> {stop_, step_};
    }

    [[nodiscard]] constexpr auto size() const -> range_difference_t<value_type>
        requires explicitly_convertible_to<value_type, range_difference_t<value_type>>
    {
        return range_step_size(start_, stop_, step_);
    }

    [[nodiscard]] constexpr auto empty() const noexcept(noexcept(begin() == end()))
        -> bool {
        return begin() == end();
    }

    [[nodiscard]] constexpr auto format() const -> std::string {
        return fmt::format("range({}, {}, {})", start_, stop_, step_);
    }

   private:
    static constexpr auto zero_ = range_type_zero_value<value_type>;
    static constexpr auto one_ = range_type_one_value<value_type>;
    value_type start_ {zero_};
    value_type stop_ {zero_};
    value_type step_ {one_};
};

}  // namespace detail

template <typename T>
using forward_range_t = detail::range_t<T, true>;
template <typename T>
using reverse_range_t = detail::range_t<T, false>;

template <typename T>
concept range_value_type =
    std::weakly_incrementable<T> && std::equality_comparable<T> &&
    std::totally_ordered<T> && std::input_iterator<detail::range_iterator_t<T, true>>;

template <range_value_type T, bool forward = true>
[[nodiscard]] constexpr auto range(T stop) noexcept(
    std::is_nothrow_constructible_v<detail::range_t<T, forward>, T>)
    -> detail::range_t<T, forward> {
    return detail::range_t<T, forward> {stop};
}

template <range_value_type T, bool forward = true>
[[nodiscard]] constexpr auto range(T start, T stop) noexcept(
    std::is_nothrow_constructible_v<detail::range_t<T, forward>, T, T>)
    -> detail::range_t<T, forward> {
    return detail::range_t<T, forward> {start, stop};
}

template <range_value_type T, bool forward = false>
[[nodiscard]] constexpr auto reverse_range(T stop) noexcept(
    std::is_nothrow_constructible_v<detail::range_t<T, forward>, T, T>)
    -> detail::range_t<T, forward> {
    return detail::range_t<T, forward> {stop, 0};
}

template <range_value_type T, bool forward = false>
[[nodiscard]] constexpr auto reverse_range(T start, T stop) noexcept(
    std::is_nothrow_constructible_v<detail::range_t<T, forward>, T, T>)
    -> detail::range_t<T, forward> {
    return detail::range_t<T, forward> {stop, start};
}

template <range_value_type T>
[[nodiscard]] constexpr auto range(T start, T stop, T step) -> detail::range_step_t<T> {
    return detail::range_step_t<T> {start, stop, step};
}

}  // namespace logicsim

template <typename T, bool forward>
inline constexpr bool std::ranges::enable_view<logicsim::detail::range_t<T, forward>> =
    true;

template <typename T, bool forward>
inline constexpr bool
    std::ranges::enable_borrowed_range<logicsim::detail::range_t<T, forward>> = true;

template <typename T>
inline constexpr bool std::ranges::enable_view<logicsim::detail::range_step_t<T>> = true;

template <typename T>
inline constexpr bool
    std::ranges::enable_borrowed_range<logicsim::detail::range_step_t<T>> = true;

//
// formatters
//

template <typename T, bool forward>
struct fmt::formatter<logicsim::detail::range_t<T, forward>> {
    constexpr auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const logicsim::detail::range_t<T, forward>& obj,
                fmt::format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

template <typename T>
struct fmt::formatter<logicsim::detail::range_step_t<T>> {
    constexpr auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const logicsim::detail::range_step_t<T>& obj,
                fmt::format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

#endif
