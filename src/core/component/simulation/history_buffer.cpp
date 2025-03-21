#include "core/component/simulation/history_buffer.h"

#include "core/allocated_size/folly_small_vector.h"
#include "core/component/simulation/history_index.h"

namespace logicsim {

namespace simulation {

HistoryBuffer::HistoryBuffer(std::initializer_list<value_type> list) : buffer_ {list} {
    if (!std::ranges::is_sorted(list, std::ranges::less_equal {})) [[unlikely]] {
        throw std::runtime_error("times need to be sorted strictly ascending");
    }
}

auto HistoryBuffer::allocated_size() const -> std::size_t {
    return get_allocated_size(buffer_.buffer());
}

auto HistoryBuffer::empty() const -> bool {
    return buffer_.empty();
}

auto HistoryBuffer::size() const -> std::size_t {
    return buffer_.size();
}

auto HistoryBuffer::ssize() const -> std::ptrdiff_t {
    return buffer_.ssize();
}

auto HistoryBuffer::pop_front() -> void {
    buffer_.pop_front();
}

auto HistoryBuffer::push_back(time_t transition_time) -> void {
    if (!empty() && transition_time <= back()) [[unlikely]] {
        throw std::runtime_error("cannot add transition time in the past");
    }
    return buffer_.push_back(transition_time);
}

auto HistoryBuffer::at(history_index_t index) const -> const time_t& {
    return buffer_.at(std::size_t {index});
}

auto HistoryBuffer::front() const -> const time_t& {
    return buffer_.front();
}

auto HistoryBuffer::back() const -> const time_t& {
    return buffer_.back();
}

auto HistoryBuffer::begin() const -> const_iterator {
    return buffer_.begin();
}

auto HistoryBuffer::end() const -> const_iterator {
    return buffer_.end();
}

}  // namespace simulation

}  // namespace logicsim
