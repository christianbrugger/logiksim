
#ifndef LOGIKSIM_CIRCULAR_BUFFER_H
#define LOGIKSIM_CIRCULAR_BUFFER_H

#include "exceptions.h"

#include <folly/small_vector.h>

namespace logicsim {

template <typename Value, std::size_t RequestedMaxInline = 1,
          typename InternalSizeType = std::size_t>
class circular_buffer {
   private:
    using buffer_t = folly::small_vector<Value, RequestedMaxInline, InternalSizeType>;
    // folly::small_vector_policy::NoHeap>;

   public:
    using size_type = std::size_t;
    using value_type = Value;
    using allocator_type = std::allocator<Value>;
    using reference = value_type&;
    using pointer = value_type*;
    using const_reference = const value_type&;
    using const_pointer = const value_type*;
    using difference_type = std::ptrdiff_t;

    // TODO:
    //  iterator
    //  const_iterator
    //  reverse_iterator
    //  const_reverse_iterator

    circular_buffer() = default;

    explicit circular_buffer(size_type n) : buffer_(std::max(n, RequestedMaxInline)) {};

    explicit circular_buffer(size_type n, value_type const& t)
        : buffer_(std::max(n, RequestedMaxInline), t) {}

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

    auto reserve(size_type new_size) -> void {
        if (new_size <= buffer_.size()) {
            return;
        }
        buffer_t new_buffer(std::max(new_size, computeNewSize()));
        assert_ls(new_buffer.size() == new_buffer.capacity());

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
        assert_ls(size() > 0);
        --size_;
    }

    auto pop_front() -> void {
        assert_ls(size() > 0);
        start_ = wrap_plus(start_, 1);
        --size_;
    }

    [[nodiscard]] auto operator[](size_type i) -> reference {
        assert_ls(i < size());
        return buffer_[wrap_plus(start_, static_cast<InternalSizeType>(i))];
    }

    [[nodiscard]] auto operator[](size_type i) const -> const_reference {
        assert_ls(i < size());
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

   private:
    /*
     * Compute the size after growth. From small_vector implementation.
     */
    [[nodiscard]] size_type computeNewSize() const {
        return std::min((3 * buffer_.capacity()) / 2 + 1, buffer_.max_size());
    }

    [[nodiscard]] auto wrap_plus(InternalSizeType a, InternalSizeType b) noexcept
        -> InternalSizeType {
        if (a + b >= buffer_.size()) {
            return static_cast<InternalSizeType>(a + b - buffer_.size());
        }
        return a + b;
    }

    [[nodiscard]] auto wrap_minus(InternalSizeType a, InternalSizeType b) noexcept
        -> InternalSizeType {
        if (b > a) {
            return static_cast<InternalSizeType>(buffer_.size() + a - b);
        }
        return a - b;
    }

    [[nodiscard]] auto get_end() noexcept -> InternalSizeType {
        return wrap_plus(start_, size_);
    }

    InternalSizeType start_ {0};
    InternalSizeType size_ {0};
    buffer_t buffer_ = buffer_t(RequestedMaxInline);
};

}  // namespace logicsim

#endif
