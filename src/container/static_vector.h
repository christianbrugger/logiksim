#ifndef LOGICSIM_CONTAINER_STATIC_VECTOR_H
#define LOGICSIM_CONTAINER_STATIC_VECTOR_H

#include <algorithm>
#include <array>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <stdexcept>

namespace logicsim {

/**
 * @brief: A constexpr buffer allocated on the heap.
 */
template <typename Value, std::size_t Capacity = 1,
          typename InternalSizeType = std::size_t>
class static_vector {
   public:
    using internal_size_t = InternalSizeType;

    using value_type = Value;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using pointer = value_type*;
    using const_reference = const value_type&;
    using const_pointer = const value_type*;

   private:
    static_assert(std::is_unsigned_v<internal_size_t>);
    static_assert(Capacity <= std::numeric_limits<internal_size_t>::max());
    static_assert(std::numeric_limits<internal_size_t>::max() <=
                  std::numeric_limits<size_type>::max());

    using buffer_t = std::array<Value, Capacity>;

    buffer_t buffer_;
    internal_size_t size_;

   public:
    using iterator = typename buffer_t::iterator;
    using const_iterator = typename buffer_t::const_iterator;
    using reverse_iterator = typename buffer_t::reverse_iterator;
    using const_reverse_iterator = typename buffer_t::const_reverse_iterator;

   public:
    [[nodiscard]] constexpr static_vector() = default;
    [[nodiscard]] explicit constexpr static_vector(size_type n);
    [[nodiscard]] constexpr static_vector(size_type n, value_type const& t);
    [[nodiscard]] constexpr static_vector(std::initializer_list<value_type> list);

    [[nodiscard]] constexpr auto empty() const noexcept -> bool;
    [[nodiscard]] constexpr auto size() const noexcept -> size_type;
    [[nodiscard]] constexpr static auto capacity() -> size_type;
    [[nodiscard]] constexpr static auto max_size() -> size_type;

    constexpr auto clear() noexcept -> void;
    constexpr auto push_back(const value_type& value) -> void;
    constexpr auto pop_back() -> void;

    [[nodiscard]] constexpr auto front() -> reference;
    [[nodiscard]] constexpr auto front() const -> const_reference;
    [[nodiscard]] constexpr auto back() -> reference;
    [[nodiscard]] constexpr auto back() const -> const_reference;
    [[nodiscard]] constexpr auto operator[](size_type i) -> reference;
    [[nodiscard]] constexpr auto operator[](size_type i) const -> const_reference;
    [[nodiscard]] constexpr auto at(size_type i) -> reference;
    [[nodiscard]] constexpr auto at(size_type i) const -> const_reference;
    [[nodiscard]] constexpr auto data() noexcept -> pointer;
    [[nodiscard]] constexpr auto data() const noexcept -> const_pointer;

    [[nodiscard]] constexpr auto begin() noexcept -> iterator;
    [[nodiscard]] constexpr auto end() noexcept -> iterator;
    [[nodiscard]] constexpr auto begin() const noexcept -> const_iterator;
    [[nodiscard]] constexpr auto end() const noexcept -> const_iterator;
    [[nodiscard]] constexpr auto cbegin() const noexcept -> const_iterator;
    [[nodiscard]] constexpr auto cend() const noexcept -> const_iterator;
};

template <typename T1, std::size_t N1, typename S1,  //
          typename T2, std::size_t N2, typename S2>
[[nodiscard]] constexpr auto operator==(const static_vector<T1, N1, S1>& left,
                                        const static_vector<T2, N2, S2>& right) -> bool;

template <typename T1, std::size_t N1, typename S1,  //
          typename T2, std::size_t N2, typename S2>
[[nodiscard]] constexpr auto operator<=>(const static_vector<T1, N1, S1>& left,
                                         const static_vector<T2, N2, S2>& right);

//
// Implementation
//

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr static_vector<Value, Capacity, SizeType>::static_vector(size_type n)
    : static_vector(n, Value()) {};

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr static_vector<Value, Capacity, SizeType>::static_vector(size_type n,
                                                                  value_type const& t) {
    if (n > capacity()) {
        throw std::runtime_error("static_vector: not enough capacity");
    }
    size_ = static_cast<internal_size_t>(n);
    std::ranges::fill(*this, t);
    std::ranges::fill(end(), buffer_.end(), Value());
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr static_vector<Value, Capacity, SizeType>::static_vector(
    std::initializer_list<value_type> list) {
    if (list.size() > capacity()) {
        throw std::runtime_error("static_vector: not enough capacity");
    }
    size_ = static_cast<internal_size_t>(list.size());
    std::ranges::copy(list, begin());
    std::ranges::fill(end(), buffer_.end(), Value());
}

//

template <typename T1, std::size_t N1, typename S1,  //
          typename T2, std::size_t N2, typename S2>
[[nodiscard]] constexpr auto operator==(const static_vector<T1, N1, S1>& left,
                                        const static_vector<T2, N2, S2>& right) -> bool {
    return std::ranges::equal(left, right);
}

template <typename T1, std::size_t N1, typename S1,  //
          typename T2, std::size_t N2, typename S2>
[[nodiscard]] constexpr auto operator<=>(const static_vector<T1, N1, S1>& left,
                                         const static_vector<T2, N2, S2>& right) {
    return std::lexicographical_compare_three_way(left.begin(), left.end(), right.begin(),
                                                  right.end());
}

//

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::empty() const noexcept -> bool {
    return size_ == 0;
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::size() const noexcept
    -> size_type {
    return size_;
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::capacity() -> size_type {
    return Capacity;
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::max_size() -> size_t {
    return Capacity;
}

//

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::clear() noexcept -> void {
    size_ = 0;
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::push_back(
    const value_type& value) -> void {
    if (size_ == capacity()) {
        throw std::runtime_error("static_vector: not enough capacity for push");
    }
    buffer_[size_] = value;
    ++size_;
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::pop_back() -> void {
    if (empty()) {
        throw std::runtime_error("static_vector: pop from empty vector");
    }
    --size_;
}

//

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::front() -> reference {
    assert(!empty());
    return buffer_[0];
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::front() const
    -> const_reference {
    assert(!empty());
    return buffer_[0];
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::back() -> reference {
    assert(!empty());
    return buffer_[size_ - 1];
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::back() const -> const_reference {
    assert(!empty());
    return buffer_[size_ - 1];
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::operator[](size_type i)
    -> reference {
    assert(i < size());
    return buffer_[i];
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::operator[](size_type i) const
    -> const_reference {
    assert(i < size());
    return buffer_[i];
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::at(size_type i) -> reference {
    if (i >= size()) {
        throw std::out_of_range("static_vector: index out of range.");
    }
    return operator[](i);
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::at(size_type i) const
    -> const_reference {
    if (i >= size()) {
        throw std::out_of_range("static_vector: index out of range.");
    }
    return operator[](i);
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::data() noexcept -> pointer {
    return buffer_.data();
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::data() const noexcept
    -> const_pointer {
    return buffer_.data();
}

//

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::begin() noexcept -> iterator {
    return buffer_.begin();
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::end() noexcept -> iterator {
    return std::next(buffer_.begin(), size_);
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::begin() const noexcept
    -> const_iterator {
    return buffer_.cbegin();
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::end() const noexcept
    -> const_iterator {
    return std::next(buffer_.cbegin(), size_);
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::cbegin() const noexcept
    -> const_iterator {
    return begin();
}

template <typename Value, std::size_t Capacity, typename SizeType>
constexpr auto static_vector<Value, Capacity, SizeType>::cend() const noexcept
    -> const_iterator {
    return end();
}

}  // namespace logicsim

#endif
