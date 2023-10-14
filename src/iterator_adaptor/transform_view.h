#ifndef LOGICSIM_ITERATOR_ADAPTOR_TRANSFORM_VIEW_H
#define LOGICSIM_ITERATOR_ADAPTOR_TRANSFORM_VIEW_H

#include "type_trait/const_iterator.h"

#include <cassert>
#include <functional>  // std::invoke
#include <iterator>
#include <ranges>
#include <type_traits>

namespace logicsim {

namespace detail {

template <bool Const, std::input_iterator I, std::copy_constructible Proj>
class TransformIterator {
    using P = std::conditional_t<Const, const Proj, Proj>;

   public:
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;

    using reference = std::indirect_result_t<Proj &, I>;
    using value_type = std::remove_cvref_t<reference>;
    using difference_type = std::iter_difference_t<I>;
    using pointer = void;

    // needs to be default constructable, so ElementView can become a range and view
    TransformIterator() = default;

    [[nodiscard]] explicit TransformIterator(P &proj, I iterator)
        : proj_ {&proj}, iterator_ {iterator} {};

    [[nodiscard]] auto operator*() const -> reference {
        assert(proj_);
        return std::invoke(*proj_, *iterator_);
    }

    // Prefix increment
    auto operator++() noexcept(noexcept(++iterator_)) -> TransformIterator & {
        ++iterator_;
        return *this;
    }

    // Postfix increment
    auto operator++(int) noexcept(noexcept(iterator_++)) -> TransformIterator {
        auto tmp = *this;
        operator++();
        return tmp;
    }

    [[nodiscard]] auto operator==(const TransformIterator &right) const
        noexcept(noexcept(this->iterator_ == right.iterator_)) -> bool {
        return this->iterator_ == right.iterator_;
    }

    [[nodiscard]] auto operator-(const TransformIterator &right) const
        noexcept(noexcept(this->iterator_ - right.iterator_)) -> difference_type
        requires requires(I it_) { it_ - it_; }
    {
        return this->iterator_ - right.iterator_;
    }

   private:
    P *proj_ {};
    I iterator_ {};
};

template <std::input_iterator I, std::copy_constructible Proj>
class TransformView {
   public:
    using iterator = TransformIterator<false, I, Proj>;
    using const_iterator = TransformIterator<true, I, Proj>;
    using iterator_type = iterator;

    using value_type = typename iterator::value_type;
    using pointer = typename iterator::pointer;
    using reference = typename iterator::reference;

    [[nodiscard]] explicit TransformView(I begin, I end, Proj proj)
        : begin_ {begin}, end_ {end}, proj_ {std::move(proj)} {};

    [[nodiscard]] auto begin() -> iterator {
        return iterator {proj_, begin_};
    }

    [[nodiscard]] auto end() -> iterator {
        return iterator {proj_, end_};
    }

    [[nodiscard]] auto begin() const -> const_iterator {
        return const_iterator {proj_, begin_};
    }

    [[nodiscard]] auto end() const -> const_iterator {
        return const_iterator {proj_, end_};
    }

    [[nodiscard]] auto size() const {
        return end_ - begin_;
    }

    [[nodiscard]] auto empty() const {
        return begin_ == end_;
    }

    static_assert(std::forward_iterator<iterator>);
    static_assert(std::forward_iterator<const_iterator>);

   private:
    I begin_;
    I end_;
    Proj proj_;
};

template <std::ranges::input_range R, std::copy_constructible Proj>
class TransformRange {
   public:
    using iterator = TransformIterator<false, std::ranges::iterator_t<R>, Proj>;
    using const_iterator = TransformIterator<true, const_iterator_t<R>, Proj>;
    using iterator_type = iterator;

    using value_type = typename iterator::value_type;
    using pointer = typename iterator::pointer;
    using reference = typename iterator::reference;

    [[nodiscard]] explicit TransformRange(R &&range, Proj proj)
        : range_ {std::move(range)}, proj_ {std::move(proj)} {};

    [[nodiscard]] auto begin() -> iterator {
        return iterator {proj_, std::ranges::begin(range_)};
    }

    [[nodiscard]] auto end() -> iterator {
        return iterator {proj_, std::ranges::end(range_)};
    }

    [[nodiscard]] auto begin() const -> const_iterator {
        return const_iterator {proj_, std::ranges::begin(range_)};
    }

    [[nodiscard]] auto end() const -> const_iterator {
        return const_iterator {proj_, std::ranges::end(range_)};
    }

    [[nodiscard]] auto size() const {
        return std::ranges::size(range_);
    }

    [[nodiscard]] auto empty() const {
        return std::ranges::empty(range_);
    }

    static_assert(std::forward_iterator<iterator>);
    static_assert(std::forward_iterator<const_iterator>);

   private:
    R range_;
    Proj proj_;
};

}  // namespace detail

template <std::input_iterator I, std::copy_constructible Proj>
    requires std::is_object_v<Proj> &&
             std::regular_invocable<Proj &, std::iter_reference_t<I>>
[[nodiscard]] auto transform_view(I begin, I end, Proj proj) {
    using view_t = detail::TransformView<I, Proj>;
    static_assert(std::ranges::forward_range<view_t>);

    return view_t {begin, end, proj};
}

template <std::ranges::input_range R, std::copy_constructible Proj>
    requires std::is_object_v<Proj> &&
             std::regular_invocable<Proj &, std::ranges::range_reference_t<R>>
[[nodiscard]] auto transform_view(R &range, Proj proj) {
    using I = decltype(std::ranges::begin(range));
    using view_t = detail::TransformView<I, Proj>;
    static_assert(std::ranges::forward_range<view_t>);

    return view_t {std::ranges::begin(range), std::ranges::end(range), proj};
}

template <std::ranges::input_range R, std::copy_constructible Proj>
    requires std::is_object_v<Proj> &&
             std::regular_invocable<Proj &&, std::ranges::range_reference_t<R>>
[[nodiscard]] auto transform_view(R &&range, Proj proj) {
    static_assert(!std::is_lvalue_reference_v<R>, "bad call");

    using range_t = detail::TransformRange<R, Proj>;
    static_assert(std::ranges::forward_range<range_t>);

    return range_t {std::move(range), proj};
}

}  // namespace logicsim

template <typename I, class Proj>
inline constexpr bool std::ranges::enable_view<logicsim::detail::TransformView<I, Proj>> =
    true;

template <typename I, class Proj>
inline constexpr bool
    std::ranges::enable_borrowed_range<logicsim::detail::TransformView<I, Proj>> = true;

#endif
