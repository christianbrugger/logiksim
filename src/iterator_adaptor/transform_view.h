#ifndef LOGICSIM_ITERATOR_ADAPTOR_TRANSFORM_VIEW_H
#define LOGICSIM_ITERATOR_ADAPTOR_TRANSFORM_VIEW_H

#include <cassert>
#include <functional>  // std::invoke
#include <iterator>
#include <ranges>
#include <type_traits>

namespace logicsim {

template <typename Iterator, class Proj>
class TransformView {
   public:
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

        [[nodiscard]] explicit TransformIterator(const TransformView &parent,
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
        const TransformView *parent_ {};
        Iterator iterator_ {};
    };

   public:
    using iterator_type = TransformIterator;

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

   private:
    Iterator begin_;
    Iterator end_;
    Proj proj_;
};

template <typename Iterator, class Proj>
[[nodiscard]] auto transform_view(Iterator begin, Iterator end, Proj proj) {
    return TransformView {begin, end, proj};
}

template <class Proj>
[[nodiscard]] auto transform_view(std::ranges::input_range auto &&range, Proj proj) {
    return TransformView<decltype(std::ranges::begin(range)), Proj> {
        std::ranges::begin(range), std::ranges::end(range), proj};
}

}  // namespace logicsim

template <typename Iterator, class Proj>
inline constexpr bool std::ranges::enable_view<logicsim::TransformView<Iterator, Proj>> =
    true;

template <typename Iterator, class Proj>
inline constexpr bool
    std::ranges::enable_borrowed_range<logicsim::TransformView<Iterator, Proj>> = true;

#endif
