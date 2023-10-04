#ifndef LOGICSIM_ALGORITHM_RANGE_H
#define LOGICSIM_ALGORITHM_RANGE_H

#include "concept/explicitly_convertible.h"
#include "concept/range_value_type.h"
#include "format/struct.h"
#include "type_trait/safe_difference_type.h"

#include <fmt/core.h>
#include <gsl/gsl>

#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace logicsim {

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
    using difference_type = safe_difference_t<T>;
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

//
// range_t
//

template <typename T, bool forward>
struct range_t {
   public:
    using value_type = std::remove_cvref_t<T>;
    using pointer = value_type*;
    using reference = value_type&;

    using iterator = range_iterator_t<value_type, forward>;
    using difference_type = iterator::difference_type;

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

    [[nodiscard]] constexpr auto size() const -> difference_type
        requires explicitly_convertible_to<value_type, difference_type>
    {
        using Dt = difference_type;

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
    static constexpr auto zero_ = value_type {0};
    value_type start_ {zero_};
    value_type stop_ {zero_};
};

}  // namespace detail

template <typename T>
using forward_range_t = detail::range_t<T, true>;
template <typename T>
using reverse_range_t = detail::range_t<T, false>;

//
// range(...)
//

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

//
// reverse_range(...)
//

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

}  // namespace logicsim

template <typename T, bool forward>
inline constexpr bool std::ranges::enable_view<logicsim::detail::range_t<T, forward>> =
    true;

template <typename T, bool forward>
inline constexpr bool
    std::ranges::enable_borrowed_range<logicsim::detail::range_t<T, forward>> = true;

#endif
