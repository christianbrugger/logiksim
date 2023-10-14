#ifndef LOGICSIM_ITERATOR_ADAPTOR_TRANSFORM_VIEW_H
#define LOGICSIM_ITERATOR_ADAPTOR_TRANSFORM_VIEW_H

#include <cassert>
#include <functional>  // std::invoke
#include <iterator>
#include <ranges>
#include <type_traits>

namespace logicsim {

namespace detail {

template <typename Iterator, class Proj>
class TransformView;

template <typename Iterator, class Proj>
class TransformIterator {
   public:
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;

    using reference = std::indirect_result_t<Proj, Iterator>;
    using value_type = std::remove_cvref_t<reference>;
    using difference_type = typename std::conditional_t<std::is_pointer_v<Iterator>,
                                                        std::pointer_traits<Iterator>,
                                                        Iterator>::difference_type;
    using pointer = void;

    // needs to be default constructable, so ElementView can become a range and view
    TransformIterator() = default;

    [[nodiscard]] explicit TransformIterator(const TransformView<Iterator, Proj> &parent,
                                             Iterator iterator)
        : parent_ {&parent}, iterator_ {iterator} {};

    [[nodiscard]] auto operator*() const -> reference {
        assert(parent_ != nullptr);
        return std::invoke(parent_->proj_, *iterator_);
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
        requires requires(Iterator it_) { it_ - it_; }
    {
        return this->iterator_ - right.iterator_;
    }

   private:
    const TransformView<Iterator, Proj> *parent_ {};
    Iterator iterator_ {};
};

template <typename Iterator, class Proj>
class TransformView {
   public:
    friend class TransformIterator<Iterator, Proj>;

   public:
    using iterator_type = TransformIterator<Iterator, Proj>;

    using value_type = typename iterator_type::value_type;
    using pointer = typename iterator_type::pointer;
    using reference = typename iterator_type::reference;

    using iterator = iterator_type;
    using const_iterator = iterator_type;

    [[nodiscard]] explicit TransformView(Iterator begin, Iterator end, Proj proj)
        : begin_ {begin}, end_ {end}, proj_ {proj} {};

    [[nodiscard]] auto begin() const {
        return iterator_type {*this, begin_};
    }

    [[nodiscard]] auto end() const -> iterator_type {
        return iterator_type {*this, end_};
    }

    [[nodiscard]] auto size() const noexcept(noexcept(end_ - begin_)) ->
        typename iterator::difference_type
        requires requires(Iterator it_) { it_ - it_; }
    {
        return end_ - begin_;
    }

    [[nodiscard]] auto empty() const noexcept(noexcept(begin_ == end_)) -> bool {
        return begin_ == end_;
    }

    static_assert(std::forward_iterator<iterator_type>);

   private:
    Iterator begin_;
    Iterator end_;
    Proj proj_;
};

}  // namespace detail

template <std::input_iterator Iterator, std::copy_constructible Proj>
    requires std::is_object_v<Proj> &&
             std::regular_invocable<Proj &, std::iter_reference_t<Iterator>>
[[nodiscard]] auto transform_view(Iterator begin, Iterator end, Proj proj) {
    using view_t = detail::TransformView<Iterator, Proj>;
    static_assert(std::ranges::forward_range<view_t>);

    return view_t {begin, end, proj};
}

template <std::ranges::input_range R, std::copy_constructible Proj>
    requires std::is_object_v<Proj> &&
             std::regular_invocable<Proj &, std::ranges::range_reference_t<R>>
[[nodiscard]] auto transform_view(R &&range, Proj proj) {
    using Iterator = decltype(std::ranges::begin(range));
    using view_t = detail::TransformView<decltype(std::ranges::begin(range)), Proj>;
    static_assert(std::ranges::forward_range<view_t>);

    return view_t {std::ranges::begin(range), std::ranges::end(range), proj};
}

}  // namespace logicsim

template <typename Iterator, class Proj>
inline constexpr bool
    std::ranges::enable_view<logicsim::detail::TransformView<Iterator, Proj>> = true;

template <typename Iterator, class Proj>
inline constexpr bool
    std::ranges::enable_borrowed_range<logicsim::detail::TransformView<Iterator, Proj>> =
        true;

#endif
