#ifndef LOGICSIM_ALGORITHM_RANGE_STEP_H
#define LOGICSIM_ALGORITHM_RANGE_STEP_H

#include "concept/explicitly_convertible.h"
#include "concept/range_value_type.h"
#include "format/struct.h"
#include "type_trait/safe_difference_type.h"

#include <fmt/core.h>

#include <iterator>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace logicsim {

namespace detail {

//
// range_iterator_step_t
//

// calculate the size of a range
template <typename T>
auto range_step_size(T start_, T stop_, T step_) -> safe_difference_t<T> {
    using Dt = safe_difference_t<T>;

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
    using difference_type = safe_difference_t<T>;
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

    using iterator = range_iterator_step_t<value_type>;
    using difference_type = iterator::difference_type;

    range_step_t() = default;

    [[nodiscard]] constexpr explicit range_step_t(value_type start, value_type stop,
                                                  value_type step)
        : start_ {std::move(start)}, stop_ {std::move(stop)}, step_ {std::move(step)} {
        if (step_ == zero_) {
            throw std::domain_error("Step cannot be zero.");
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

    [[nodiscard]] constexpr auto size() const -> difference_type
        requires explicitly_convertible_to<value_type, difference_type>
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
    static constexpr auto zero_ = value_type {0};
    static constexpr auto one_ = value_type {1};
    value_type start_ {zero_};
    value_type stop_ {zero_};
    value_type step_ {one_};
};

}  // namespace detail

template <range_value_type T>
[[nodiscard]] constexpr auto range(T start, T stop, T step) -> detail::range_step_t<T> {
    return detail::range_step_t<T> {start, stop, step};
}

}  // namespace logicsim

template <typename T>
inline constexpr bool std::ranges::enable_view<logicsim::detail::range_step_t<T>> = true;

template <typename T>
inline constexpr bool
    std::ranges::enable_borrowed_range<logicsim::detail::range_step_t<T>> = true;

#endif
