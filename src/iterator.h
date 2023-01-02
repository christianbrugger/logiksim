
#ifndef LOGIKSIM_ITERATOR_H
#define LOGIKSIM_ITERATOR_H

#include <iterator>
#include <ranges>
#include <type_traits>

namespace logicsim {

template <typename Iterator, class Proj>
class TransformIterator {
   public:
    using iterator_concept = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;

    using value_type = typename std::projected<Iterator, Proj>::value_type;
    using difference_type = typename Iterator::difference_type;
    using pointer = value_type *;
    using reference = value_type &;

    // needs to be default constructable, so ElementView can become a range and view
    TransformIterator() = default;

    [[nodiscard]] explicit TransformIterator(Iterator iterator, Proj proj) noexcept(
        std::is_nothrow_copy_constructible_v<Iterator>
            &&std::is_nothrow_copy_constructible_v<Proj>)
        : iterator_ {iterator}, proj_ {proj} {};

    [[nodiscard]] auto operator*() const -> value_type { return proj_(*iterator_); }

    // Prefix increment
    auto operator++() noexcept(noexcept(++iterator_)) -> TransformIterator & {
        ++iterator_;
        return *this;
    }

    // Postfix increment
    auto operator++(int) noexcept(noexcept(iterator_++)) -> TransformIterator {
        auto tmp = *this;
        return ++iterator_;
        return tmp;
    }

    [[nodiscard]] auto operator==(const TransformIterator &right) const
        noexcept(noexcept(this->iterator_ == right.iterator_)) -> bool {
        return this->iterator_ == right.iterator_;
    }

    [[nodiscard]] auto operator-(const TransformIterator &right) const
        noexcept(noexcept(this->iterator_ - right.iterator_)) -> difference_type {
        return this->iterator_ - right.iterator_;
    }

   private:
    Iterator iterator_ {};
    Proj proj_ {};
};

template <typename Iterator, class Proj>
class TransformView {
   public:
    using iterator_type = TransformIterator<Iterator, Proj>;

    using value_type = typename iterator_type::value_type;
    using pointer = typename iterator_type::pointer;
    using reference = typename iterator_type::reference;

    using iterator = iterator_type;
    using const_iterator = iterator_type;

    [[nodiscard]] explicit TransformView(
        Iterator begin, Iterator end,
        Proj proj) noexcept(std::is_nothrow_copy_constructible_v<Iterator>
                                &&std::is_nothrow_copy_constructible_v<Proj>)
        : begin_ {begin}, end_ {end}, proj_ {proj} {};

    [[nodiscard]] auto begin() const
        noexcept(std::is_nothrow_constructible_v<iterator_type, Iterator, Proj>)
            -> iterator_type {
        return iterator_type {begin_, proj_};
    }

    [[nodiscard]] auto end() const
        noexcept(std::is_nothrow_constructible_v<iterator_type, Iterator, Proj>)
            -> iterator_type {
        return iterator_type {end_, proj_};
    }

    [[nodiscard]] auto size() const noexcept(noexcept(end_ - begin_)) ->
        typename iterator::difference_type {
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
[[nodiscard]] auto transform_view(Iterator begin, Iterator end, Proj proj) noexcept(
    std::is_nothrow_constructible_v<TransformView<Iterator, Proj>, Iterator, Iterator,
                                    Proj>) {
    return TransformView {begin, end, proj};
}

template <class Proj>
[[nodiscard]] auto
transform_view(std::ranges::input_range auto &&range, Proj proj) noexcept(
    std::is_nothrow_constructible_v<
        TransformView<decltype(std::ranges::begin(range)), Proj>,
        decltype(std::ranges::begin(range)), decltype(std::ranges::begin(range)), Proj>) {
    return TransformView<decltype(std::ranges::begin(range)), Proj> {
        std::ranges::begin(range), std::ranges::end(range), proj};
}

}  // namespace logicsim

template <typename Iterator, class Proj>
inline constexpr bool std::ranges::enable_view<logicsim::TransformView<Iterator, Proj>>
    = true;

template <typename Iterator, class Proj>
inline constexpr bool
    std::ranges::enable_borrowed_range<logicsim::TransformView<Iterator, Proj>>
    = true;

#endif
