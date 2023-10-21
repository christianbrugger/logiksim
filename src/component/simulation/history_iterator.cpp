#include "component/simulation/history_iterator.h"

namespace logicsim {

namespace simulation {

HistoryIterator::HistoryIterator(HistoryView view, history_index_t index) noexcept
    : view_ {std::move(view)}, index_ {index} {}

auto HistoryIterator::operator*() const -> value_type {
    return history_entry_t {
        view_.get_time(static_cast<std::ptrdiff_t>(std::size_t {index_}) - 1),
        view_.get_time(static_cast<std::ptrdiff_t>(std::size_t {index_})),
        view_.get_value(static_cast<std::ptrdiff_t>(std::size_t {index_})),
    };
}

auto HistoryIterator::operator++() noexcept -> HistoryIterator & {
    ++index_;
    return *this;
}

auto HistoryIterator::operator++(int) noexcept -> HistoryIterator {
    const auto tmp = *this;
    ++(*this);
    return tmp;
}

auto HistoryIterator::operator==(const HistoryIterator &right) const noexcept -> bool {
    return index_ >= right.index_;
}

auto HistoryIterator::operator-(const HistoryIterator &right) const noexcept
    -> difference_type {
    return static_cast<std::ptrdiff_t>(std::size_t {index_}) -
           static_cast<std::ptrdiff_t>(std::size_t {right.index_});
}

}  // namespace simulation

}  // namespace logicsim
