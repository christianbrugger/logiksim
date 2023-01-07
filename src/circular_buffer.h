
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

    auto size() const -> size_type {
        return size_;
    }

    auto reserve(size_type new_size) -> void {
        if (new_size <= buffer_.size()) {
            return;
        }
        const auto new_size = std::max(new_size, buffer_.computeNewSize());
        buffer_t new_buffer {new_size};
        assert(new_buffer.size() == new_buffer.capacity);

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
        assert(size > 0);
        --size_;
    }

    auto pop_front() -> void {
        assert(size > 0);
        start_ = wrap(start_ + 1);
        --size_;
    }

    auto capacity() const -> size_type {
        return buffer_.size();
    }

    reference operator[](size_type i) {
        assert(i < size());
        return buffer_[wrap(start_ + i)];
    }

    const_reference operator[](size_type i) const {
        assert(i < size());
        return buffer_[wrap(start_ + i)];
    }

    reference at(size_type i) {
        if (i >= size()) {
            throw_exception("circular_buffer: index out of range.");
        }
        return (*this)[i];
    }

    const_reference at(size_type i) const {
        if (i >= size()) {
            throw_exception("circular_buffer: index out of range.");
        }
        return (*this)[i];
    }

   private:
    auto get_end() -> InternalSizeType {
        return wrap(start_ + size_);
    }

    auto wrap(InternalSizeType value) {
        // InternalSizeType is unsigned, so it cannot be negative and overflows
        return std::min(value, value - buffer_.size());
    }

    auto wrap_minus(InternalSizeType a, InternalSizeType b) {
        return std::min(a - b, buffer_.size() + a - b);
    }

    InternalSizeType start_ {0};
    InternalSizeType size_ {0};
    // InternalSizeType end_ {0};
    buffer_t buffer_ {RequestedMaxInline};
};

}  // namespace logicsim

#endif
