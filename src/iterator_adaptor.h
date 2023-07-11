
// TODO rename file to iterator_adaptors.h

#ifndef LOGIKSIM_ITERATOR_H
#define LOGIKSIM_ITERATOR_H

#include <cassert>
#include <functional>
#include <iterator>
#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>

namespace logicsim {

//
// Enumerate
//

/*
template <std::ranges::input_range T>
constexpr auto enumerate(T &&range) {
    struct Iterator {
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;

        using difference_type = std::ranges::range_difference_t<T>;
        using value_type = std::pair<difference_type, std::ranges::range_value_t<T>>;
        using reference = std::pair<difference_type, std::ranges::range_reference_t<T>>;
        using pointer = void;

        std::ranges::iterator_t<T> iterator {};
        difference_type index {};

        auto operator++() -> Iterator & {
            ++index;
            ++iterator;
            return *this;
        }

        auto operator++(int) -> Iterator {
            auto tmp = *this;
            operator++();
            return tmp;
        }

        auto operator*() const -> reference {
            return reference {index, *iterator};
        }

        auto operator==(const Iterator &other) const -> bool {
            return iterator == other.iterator;
        }
    };

    struct View {
        T iterable;

        auto begin() {
            return Iterator {std::begin(iterable)};
        }

        auto end() {
            return Iterator {std::end(iterable)};
        }
    };

    static_assert(std::forward_iterator<Iterator>);
    static_assert(std::ranges::forward_range<View>);

    return View {std::forward<T>(range)};
}
*/

//
// Transform View & Iterator
//

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
inline constexpr bool std::ranges::enable_view<logicsim::TransformView<Iterator, Proj>>
    = true;

template <typename Iterator, class Proj>
inline constexpr bool
    std::ranges::enable_borrowed_range<logicsim::TransformView<Iterator, Proj>>
    = true;

namespace logicsim {

//
// Transform Output Iterator
//

template <class Proj, typename Iterator>
class transform_output_iterator {
   public:
    using iterator_concept = std::output_iterator_tag;
    using iterator_category = std::output_iterator_tag;

    using difference_type = typename Iterator::difference_type;
    using value_type = void;
    using pointer = void;
    using reference = void;

    explicit transform_output_iterator(Proj proj, Iterator iterator)
        : iterator_(std::move(iterator)), proj_(std::move(proj)) {}

    auto operator++() -> transform_output_iterator & {
        ++iterator_;
        return *this;
    }

    auto operator++(int) -> transform_output_iterator {
        auto temp = *this;
        ++iterator_;
        return temp;
    }

    auto operator*() -> transform_output_iterator & {
        return *this;
    }

    template <typename T>
    auto operator=(T &&value) -> transform_output_iterator & {
        *iterator_ = std::invoke(proj_, std::forward<T>(value));
        return *this;
    }

   private:
    Iterator iterator_;
    Proj proj_;
};

template <typename Pred, class Proj, class Iterator>
class transform_if_output_iterator {
   public:
    using iterator_concept = std::output_iterator_tag;
    using iterator_category = std::output_iterator_tag;

    using difference_type = typename Iterator::difference_type;
    using value_type = void;
    using pointer = void;
    using reference = void;

    explicit transform_if_output_iterator(Pred pred, Proj proj, Iterator iterator)
        : iterator_(std::move(iterator)),
          proj_(std::move(proj)),
          pred_(std::move(pred)) {}

    auto operator++() -> transform_if_output_iterator & {
        ++iterator_;
        return *this;
    }

    auto operator++(int) -> transform_if_output_iterator {
        auto temp = *this;
        ++iterator_;
        return temp;
    }

    auto operator*() -> transform_if_output_iterator & {
        return *this;
    }

    template <typename T>
    auto operator=(T &&value) -> transform_if_output_iterator & {
        if (pred_(value)) {
            *iterator_ = std::invoke(proj_, std::forward<T>(value));
        }
        return *this;
    }

   private:
    Iterator iterator_;
    Proj proj_;
    Pred pred_;
};

template <typename Iterator, class Pred>
class filter_output_iterator {
   public:
    using iterator_concept = std::output_iterator_tag;
    using iterator_category = std::output_iterator_tag;

    using difference_type = typename Iterator::difference_type;
    using value_type = void;
    using pointer = void;
    using reference = void;

    explicit filter_output_iterator(Pred pred, Iterator iterator)
        : iterator_(std::move(iterator)), pred_(std::move(pred)) {}

    auto operator++() -> filter_output_iterator & {
        ++iterator_;
        return *this;
    }

    auto operator++(int) -> filter_output_iterator {
        auto temp = *this;
        ++iterator_;
        return temp;
    }

    auto operator*() -> filter_output_iterator & {
        return *this;
    }

    template <typename T>
    auto operator=(T &&value) -> filter_output_iterator & {
        if (pred_(value)) {
            *iterator_ = std::forward<T>(value);
        }
        return *this;
    }

   private:
    Iterator iterator_;
    Pred pred_;
};

//
// output callable
//

template <class Func>
class output_callable {
   public:
    using iterator_concept = std::output_iterator_tag;
    using iterator_category = std::output_iterator_tag;

    using difference_type = std::ptrdiff_t;
    using value_type = void;
    using pointer = void;
    using reference = void;

    explicit output_callable(Func func) : func_(std::move(func)) {}

    auto operator++() -> output_callable & {
        return *this;
    }

    auto operator++(int) -> output_callable {
        return *this;
    }

    auto operator*() -> output_callable & {
        return *this;
    }

    template <typename T>
    auto operator=(T &&value) -> output_callable & {
        std::invoke(func_, std::forward<T>(value));
        return *this;
    }

   private:
    Func func_;
};

}  // namespace logicsim

#endif
