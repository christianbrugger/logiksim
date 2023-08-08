
#ifndef LOGIKSIM_CIRCULAR_BUFFER_H
#define LOGIKSIM_CIRCULAR_BUFFER_H

#include "exceptions.h"

#include <folly/small_vector.h>
#include <gsl/gsl>

#include <type_traits>

namespace logicsim {

template <typename Value, std::size_t RequestedMaxInline = 1,
          typename InternalSizeType = std::size_t>
    requires std::is_trivial_v<Value>
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

    circular_buffer() = default;

    explicit circular_buffer(size_type n) : buffer_(std::max(n, RequestedMaxInline)) {};

    explicit circular_buffer(size_type n, value_type const& t)
        : buffer_(std::max(n, RequestedMaxInline), t) {}

    explicit circular_buffer(std::initializer_list<value_type> list) {
        reserve(list.size());
        std::ranges::copy(list, std::back_inserter(*this));
    }

    [[nodiscard]] auto size() const noexcept -> size_type {
        return size_;
    }

    [[nodiscard]] auto capacity() const -> size_type {
        return buffer_.size();
    }

    [[nodiscard]] auto max_size() const -> size_t {
        return buffer_.max_size();
    }

    auto clear() noexcept -> void {
        size_ = 0;
    }

    auto empty() const noexcept -> bool {
        return size_ == 0;
    }

    auto reserve(size_type new_size) -> void {
        if (new_size <= buffer_.size()) {
            return;
        }
        buffer_t new_buffer(std::max(new_size, computeNewSize()));
        assert(new_buffer.size() == new_buffer.capacity());

        // copy ring buffer to the beginning of the new buffer
        const auto half_count = buffer_.size() - start_;
        std::copy(buffer_.begin() + start_, buffer_.begin() + start_ + half_count,
                  new_buffer.begin());
        std::copy(buffer_.begin(), buffer_.begin() + start_,
                  new_buffer.begin() + half_count);

        // swap
        buffer_.swap(new_buffer);
        start_ = 0;
    }

    auto push_front(const value_type& value) -> void {
        if (size_ == buffer_.size()) {
            reserve(size_ + 1);
        }
        start_ = wrap_minus(start_, 1);
        buffer_[start_] = value;
        ++size_;
    }

    auto push_back(const value_type& value) -> void {
        if (size_ == buffer_.size()) {
            reserve(size_ + 1);
        }
        buffer_[get_end()] = value;
        ++size_;
    }

    auto pop_back() -> void {
        assert(size() > 0);
        --size_;
    }

    auto pop_front() -> void {
        assert(size() > 0);
        start_ = wrap_plus(start_, 1);
        --size_;
    }

    auto front() -> reference {
        assert(!empty());
        return buffer_[start_];
    }

    auto back() -> reference {
        assert(!empty());
        return buffer_[wrap_plus(start_, size_ - 1)];
    }

    [[nodiscard]] auto operator[](size_type i) -> reference {
        assert(i < size());
        return buffer_[wrap_plus(start_, static_cast<InternalSizeType>(i))];
    }

    [[nodiscard]] auto operator[](size_type i) const -> const_reference {
        assert(i < size());
        return buffer_[wrap_plus(start_, static_cast<InternalSizeType>(i))];
    }

    [[nodiscard]] auto at(size_type i) -> reference {
        if (i >= size()) {
            throw_exception("circular_buffer: index out of range.");
        }
        return (*this)[i];
    }

    [[nodiscard]] auto at(size_type i) const -> const_reference {
        if (i >= size()) {
            throw_exception("circular_buffer: index out of range.");
        }
        return (*this)[i];
    }

    [[nodiscard]] auto begin() -> iterator {
        return iterator {*this, 0};
    }

    [[nodiscard]] auto end() -> iterator {
        return iterator {*this, size_};
    }

    [[nodiscard]] auto begin() const -> const_iterator {
        return const_iterator {*this, 0};
    }

    [[nodiscard]] auto end() const -> const_iterator {
        return const_iterator {*this, size_};
    }

    [[nodiscard]] auto cbegin() const -> const_iterator {
        return begin();
    }

    [[nodiscard]] auto cend() const -> const_iterator {
        return end();
    }

    [[nodiscard]] auto rbegin() -> reverse_iterator {
        return reverse_iterator {end()};
    }

    [[nodiscard]] auto rend() -> reverse_iterator {
        return reverse_iterator {begin()};
    }

    [[nodiscard]] auto rbegin() const -> const_reverse_iterator {
        return const_reverse_iterator {end()};
    }

    [[nodiscard]] auto rend() const -> const_reverse_iterator {
        return const_reverse_iterator {begin()};
    }

    [[nodiscard]] auto crbegin() const -> const_reverse_iterator {
        return rbegin();
    }

    [[nodiscard]] auto crend() const -> const_reverse_iterator {
        return rend();
    }

   private:
    /// Compute the size after growth. From small_vector implementation.
    [[nodiscard]] size_type computeNewSize() const {
        return std::min((3 * buffer_.capacity()) / 2 + 1, buffer_.max_size());
    }

    [[nodiscard]] auto wrap_plus(InternalSizeType a, InternalSizeType b) const noexcept
        -> InternalSizeType {
        if (a + b >= buffer_.size()) {
            return static_cast<InternalSizeType>(a + b - buffer_.size());
        }
        return a + b;
    }

    [[nodiscard]] auto wrap_minus(InternalSizeType a, InternalSizeType b) const noexcept
        -> InternalSizeType {
        if (b > a) {
            return static_cast<InternalSizeType>(buffer_.size() + a - b);
        }
        return a - b;
    }

    [[nodiscard]] auto get_end() const noexcept -> InternalSizeType {
        return wrap_plus(start_, size_);
    }

    static_assert(std::random_access_iterator<iterator>);
    static_assert(std::random_access_iterator<const_iterator>);
};

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
    requires std::is_trivial_v<Value>
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

    Iterator() = default;

    explicit Iterator(container_t& container, std::size_t index)
        : container_ {&container}, index_ {index} {}

    auto operator*() const -> reference {
        return (*container_)[index_];
    }

    auto operator->() const -> pointer {
        return &(*(*this));
    }

    auto operator==(const Iterator& right) const -> bool {
        return index_ == right.index_;
    }

    auto operator-(const Iterator& right) const -> difference_type {
        return index_ - right.index_;
    }

    auto operator<=>(const Iterator& right) const {
        return index_ <=> right.index_;
    }

    auto operator++() -> Iterator& {
        ++index_;
        return *this;
    }

    auto operator++(int) -> Iterator {
        auto copy = *this;
        ++index_;
        return copy;
    }

    auto operator--() -> Iterator& {
        --index_;
        return *this;
    }

    auto operator--(int) -> Iterator {
        auto copy = *this;
        --index_;
        return copy;
    }

    auto operator+=(difference_type offset) -> Iterator& {
        index_ += offset;
        return *this;
    }

    auto operator-=(difference_type offset) -> Iterator& {
        index_ -= offset;
        return *this;
    }

    auto operator+(difference_type offset) const -> Iterator {
        auto copy = *this;
        return copy += offset;
    }

    friend auto operator+(difference_type offset, Iterator right) -> Iterator {
        return right += offset;
    }

    auto operator-(difference_type offset) const -> Iterator {
        auto copy = *this;
        return copy -= offset;
    }

    auto operator[](difference_type position) const -> reference {
        return *(*this + position);
    }
};

}  // namespace logicsim

template <typename Value, std::size_t RequestedMaxInline, typename InternalSizeType>
inline constexpr bool std::ranges::enable_view<
    logicsim::circular_buffer<Value, RequestedMaxInline, InternalSizeType>> = true;

#endif
