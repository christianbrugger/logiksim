
#ifndef LOGIKSIM_CIRCULAR_BUFFER_H
#define LOGIKSIM_CIRCULAR_BUFFER_H

#include <folly/small_vector.h>

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <ranges>
#include <type_traits>

namespace logicsim {

template <typename Value, std::size_t RequestedMaxInline = 1,
          typename InternalSizeType = std::size_t>
class circular_buffer {
   private:
    using policy = folly::small_vector_policy::policy_size_type<InternalSizeType>;
    using buffer_t = folly::small_vector<Value, RequestedMaxInline, policy>;

    buffer_t buffer_ = buffer_t(RequestedMaxInline);
    InternalSizeType start_ {0};
    InternalSizeType size_ {0};

    template <bool Const>
    class Iterator;

   public:
    using internal_size_t = InternalSizeType;

    using size_type = std::size_t;
    using value_type = Value;
    using allocator_type = std::allocator<Value>;
    using reference = value_type&;
    using pointer = value_type*;
    using const_reference = const value_type&;
    using const_pointer = const value_type*;
    using difference_type = std::ptrdiff_t;

    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    [[nodiscard]] explicit circular_buffer() = default;
    [[nodiscard]] explicit circular_buffer(size_type n);
    [[nodiscard]] explicit circular_buffer(size_type n, value_type const& t);
    [[nodiscard]] circular_buffer(std::initializer_list<value_type> list);

    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto size() const noexcept -> size_type;
    [[nodiscard]] auto ssize() const noexcept -> difference_type;
    [[nodiscard]] auto capacity() const -> size_type;
    [[nodiscard]] auto max_size() const -> size_t;

    auto clear() noexcept -> void;
    auto reserve(size_type new_size) -> void;

    auto push_front(const value_type& value) -> void;
    auto push_back(const value_type& value) -> void;
    auto pop_back() -> void;
    auto pop_front() -> void;

    [[nodiscard]] auto front() -> reference;
    [[nodiscard]] auto front() const -> const_reference;
    [[nodiscard]] auto back() -> reference;
    [[nodiscard]] auto back() const -> const_reference;
    [[nodiscard]] auto operator[](size_type i) -> reference;
    [[nodiscard]] auto operator[](size_type i) const -> const_reference;
    [[nodiscard]] auto at(size_type i) -> reference;
    [[nodiscard]] auto at(size_type i) const -> const_reference;

    [[nodiscard]] auto begin() -> iterator;
    [[nodiscard]] auto end() -> iterator;
    [[nodiscard]] auto begin() const -> const_iterator;
    [[nodiscard]] auto end() const -> const_iterator;
    [[nodiscard]] auto cbegin() const -> const_iterator;
    [[nodiscard]] auto cend() const -> const_iterator;

    [[nodiscard]] auto rbegin() -> reverse_iterator;
    [[nodiscard]] auto rend() -> reverse_iterator;
    [[nodiscard]] auto rbegin() const -> const_reverse_iterator;
    [[nodiscard]] auto rend() const -> const_reverse_iterator;
    [[nodiscard]] auto crbegin() const -> const_reverse_iterator;
    [[nodiscard]] auto crend() const -> const_reverse_iterator;

   private:
    /*
     * Compute the size after growth.
     */
    [[nodiscard]] auto computeNewSize() const -> size_type;
    [[nodiscard]] auto wrap_plus(InternalSizeType a,
                                 InternalSizeType b) const noexcept -> InternalSizeType;
    [[nodiscard]] auto wrap_minus(InternalSizeType a,
                                  InternalSizeType b) const noexcept -> InternalSizeType;
    [[nodiscard]] auto get_end() const noexcept -> InternalSizeType;

    static_assert(std::random_access_iterator<iterator>);
    static_assert(std::random_access_iterator<const_iterator>);
};

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
template <bool Const>
class circular_buffer<Value, RequestedMaxInline, InternalSizeType>::Iterator {
    using container_base_t = circular_buffer<Value, RequestedMaxInline, InternalSizeType>;
    using container_t =
        std::conditional_t<Const, const container_base_t, container_base_t>;

    container_t* container_ {};
    std::size_t index_ {};

   public:
    using iterator_concept = std::random_access_iterator_tag;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = std::conditional_t<Const, const Value, Value>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    [[nodiscard]] explicit Iterator() = default;
    [[nodiscard]] explicit Iterator(container_t& container, std::size_t index);

    [[nodiscard]] auto operator*() const -> reference;
    [[nodiscard]] auto operator->() const -> pointer;
    [[nodiscard]] auto operator[](difference_type position) const -> reference;

    [[nodiscard]] auto operator==(const Iterator& right) const -> bool;
    [[nodiscard]] auto operator<=>(const Iterator& right) const;
    [[nodiscard]] auto operator-(const Iterator& right) const -> difference_type;

    auto operator++() -> Iterator&;
    [[nodiscard]] auto operator++(int) -> Iterator;
    auto operator--() -> Iterator&;
    [[nodiscard]] auto operator--(int) -> Iterator;

    auto operator+=(difference_type offset) -> Iterator&;
    auto operator-=(difference_type offset) -> Iterator&;
    [[nodiscard]] auto operator+(difference_type offset) const -> Iterator;
    [[nodiscard]] auto operator-(difference_type offset) const -> Iterator;

    [[nodiscard]] friend auto operator+(difference_type offset,
                                        const Iterator& right) -> Iterator {
        auto result = right;
        result += offset;
        return result;
    }
};

//
// Circular Buffer
//

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
circular_buffer<Value, RequestedMaxInline, InternalSizeType>::circular_buffer(size_type n)
    : buffer_(std::max(n, RequestedMaxInline)) {};

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
circular_buffer<Value, RequestedMaxInline, InternalSizeType>::circular_buffer(
    size_type n, value_type const& t)
    : buffer_(std::max(n, RequestedMaxInline), t) {}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
circular_buffer<Value, RequestedMaxInline, InternalSizeType>::circular_buffer(
    std::initializer_list<value_type> list) {
    reserve(list.size());
    std::ranges::copy(list, std::back_inserter(*this));
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::size() const noexcept
    -> size_type {
    return size_;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::ssize() const noexcept
    -> difference_type {
    return static_cast<difference_type>(size_);
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::capacity() const
    -> size_type {
    return buffer_.size();
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::max_size() const
    -> size_t {
    return buffer_.max_size();
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::clear() noexcept
    -> void {
    size_ = 0;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::empty() const noexcept
    -> bool {
    return size_ == 0;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::push_front(
    const value_type& value) -> void {
    if (size_ == buffer_.size()) {
        reserve(size_ + 1);
    }
    start_ = wrap_minus(start_, 1);
    buffer_[start_] = value;
    ++size_;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::push_back(
    const value_type& value) -> void {
    if (size_ == buffer_.size()) {
        reserve(size_ + 1);
    }
    buffer_[get_end()] = value;
    ++size_;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::pop_back() -> void {
    assert(size() > 0);
    --size_;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::pop_front() -> void {
    assert(size() > 0);
    start_ = wrap_plus(start_, 1);
    --size_;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::front() -> reference {
    assert(!empty());
    return buffer_[start_];
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::front() const
    -> const_reference {
    assert(!empty());
    return buffer_[start_];
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::back() -> reference {
    assert(!empty());
    return buffer_[wrap_plus(start_, size_ - 1)];
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::back() const
    -> const_reference {
    assert(!empty());
    return buffer_[wrap_plus(start_, size_ - 1)];
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::operator[](size_type i)
    -> reference {
    assert(i < size());
    return buffer_[wrap_plus(start_, static_cast<InternalSizeType>(i))];
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::operator[](
    size_type i) const -> const_reference {
    assert(i < size());
    return buffer_[wrap_plus(start_, static_cast<InternalSizeType>(i))];
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::at(size_type i)
    -> reference {
    if (i >= size()) {
        throw std::out_of_range("circular_buffer: index out of range.");
    }
    return (*this)[i];
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::at(size_type i) const
    -> const_reference {
    if (i >= size()) {
        throw std::out_of_range("circular_buffer: index out of range.");
    }
    return (*this)[i];
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::begin() -> iterator {
    return iterator {*this, 0};
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::end() -> iterator {
    return iterator {*this, size_};
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::begin() const
    -> const_iterator {
    return const_iterator {*this, 0};
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::end() const
    -> const_iterator {
    return const_iterator {*this, size_};
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::cbegin() const
    -> const_iterator {
    return begin();
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::cend() const
    -> const_iterator {
    return end();
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::rbegin()
    -> reverse_iterator {
    return reverse_iterator {end()};
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::rend()
    -> reverse_iterator {
    return reverse_iterator {begin()};
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::rbegin() const
    -> const_reverse_iterator {
    return const_reverse_iterator {end()};
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::rend() const
    -> const_reverse_iterator {
    return const_reverse_iterator {begin()};
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::crbegin() const
    -> const_reverse_iterator {
    return rbegin();
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::crend() const
    -> const_reverse_iterator {
    return rend();
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::reserve(
    size_type new_size) -> void {
    if (new_size <= buffer_.size()) {
        return;
    }
    buffer_t new_buffer(std::max(new_size, computeNewSize()));
    assert(new_buffer.size() == new_buffer.capacity());

    // copy ring buffer to the beginning of the new buffer
    const auto half_count = buffer_.size() - start_;
    std::copy(buffer_.begin() + start_, buffer_.begin() + start_ + half_count,
              new_buffer.begin());
    std::copy(buffer_.begin(), buffer_.begin() + start_, new_buffer.begin() + half_count);

    // swap
    buffer_.swap(new_buffer);
    start_ = 0;
}

/*
 * Compute the size after growth.
 */
template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::computeNewSize() const
    -> size_type {
    // From small_vector implementation.
    return std::min((3 * buffer_.capacity()) / 2 + 1, buffer_.max_size());
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::wrap_plus(
    InternalSizeType a, InternalSizeType b) const noexcept -> InternalSizeType {
    if (a + b >= buffer_.size()) {
        return static_cast<InternalSizeType>(a + b - buffer_.size());
    }
    return a + b;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::wrap_minus(
    InternalSizeType a, InternalSizeType b) const noexcept -> InternalSizeType {
    if (b > a) {
        return static_cast<InternalSizeType>(buffer_.size() + a - b);
    }
    return a - b;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::get_end()
    const noexcept -> InternalSizeType {
    return wrap_plus(start_, size_);
}

//
// Circular Buffer Iterator
//

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
template <bool Const>
circular_buffer<Value, RequestedMaxInline, InternalSizeType>::Iterator<Const>::Iterator(
    container_t& container, std::size_t index)
    : container_ {&container}, index_ {index} {}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
template <bool Const>
auto circular_buffer<Value, RequestedMaxInline,
                     InternalSizeType>::Iterator<Const>::operator*() const -> reference {
    return container_->operator[](index_);
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
template <bool Const>
auto circular_buffer<Value, RequestedMaxInline,
                     InternalSizeType>::Iterator<Const>::operator->() const -> pointer {
    return &(operator*());
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
template <bool Const>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::Iterator<
    Const>::operator==(const Iterator& right) const -> bool {
    return index_ == right.index_;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
template <bool Const>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::Iterator<
    Const>::operator-(const Iterator& right) const -> difference_type {
    return index_ - right.index_;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
template <bool Const>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::Iterator<
    Const>::operator<=>(const Iterator& right) const {
    return index_ <=> right.index_;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
template <bool Const>
auto circular_buffer<Value, RequestedMaxInline,
                     InternalSizeType>::Iterator<Const>::operator++() -> Iterator& {
    ++index_;
    return *this;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
template <bool Const>
auto circular_buffer<Value, RequestedMaxInline,
                     InternalSizeType>::Iterator<Const>::operator++(int) -> Iterator {
    auto copy = *this;
    ++index_;
    return copy;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
template <bool Const>
auto circular_buffer<Value, RequestedMaxInline,
                     InternalSizeType>::Iterator<Const>::operator--() -> Iterator& {
    --index_;
    return *this;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
template <bool Const>
auto circular_buffer<Value, RequestedMaxInline,
                     InternalSizeType>::Iterator<Const>::operator--(int) -> Iterator {
    auto copy = *this;
    --index_;
    return copy;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
template <bool Const>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::Iterator<
    Const>::operator+=(difference_type offset) -> Iterator& {
    index_ += offset;
    return *this;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
template <bool Const>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::Iterator<
    Const>::operator-=(difference_type offset) -> Iterator& {
    index_ -= offset;
    return *this;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
template <bool Const>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::Iterator<
    Const>::operator+(difference_type offset) const -> Iterator {
    auto copy = *this;
    return copy += offset;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
template <bool Const>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::Iterator<
    Const>::operator-(difference_type offset) const -> Iterator {
    auto copy = *this;
    return copy -= offset;
}

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
template <bool Const>
auto circular_buffer<Value, RequestedMaxInline, InternalSizeType>::Iterator<
    Const>::operator[](difference_type position) const -> reference {
    return *(*this + position);
}

}  // namespace logicsim

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
inline constexpr bool std::ranges::enable_view<
    logicsim::circular_buffer<Value, RequestedMaxInline, InternalSizeType>> = true;

#endif
