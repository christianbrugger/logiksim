#include "component/simulation/history_view.h"

#include "component/simulation/history_iterator.h"  // TODO remove circular
#include "vocabulary/delay.h"

#include <gsl/gsl>

#include <algorithm>
#include <stdexcept>

namespace logicsim {

namespace simulation {

HistoryView::HistoryView(const HistoryBuffer &history, time_t simulation_time,
                         bool last_value, delay_t history_length)
    : history_ {&history}, simulation_time_ {simulation_time}, last_value_ {last_value} {
    // ascending without duplicates
    assert(std::ranges::is_sorted(history, std::ranges::less_equal {}));
    // calculate first valid index
    const auto first_time = simulation_time - history_length;
    const auto first_index = find_index(first_time);
    min_index_ = gsl::narrow<decltype(min_index_)>(first_index);

    assert(size() >= 1);
}

auto HistoryView::size() const -> std::size_t {
    if (history_ == nullptr) {
        return 1;
    }
    return history_->size() + 1 - std::size_t {min_index_};
}

auto HistoryView::ssize() const -> std::ptrdiff_t {
    if (history_ == nullptr) {
        return 1;
    }
    return history_->size() + 1 - std::size_t {min_index_};
}

auto HistoryView::begin() const -> HistoryIterator {
    return HistoryIterator {*this, min_index_};
}

auto HistoryView::end() const -> HistoryIterator {
    return HistoryIterator {*this, history_index_t {size() + std::size_t {min_index_}}};
}

auto HistoryView::from(time_t value) const -> HistoryIterator {
    if (value > simulation_time_) [[unlikely]] {
        throw std::runtime_error("cannot query times in the future");
    }
    const auto index = find_index(value);
    return HistoryIterator {*this, history_index_t {index}};
}

auto HistoryView::until(time_t value) const -> HistoryIterator {
    if (value > simulation_time_) [[unlikely]] {
        throw std::runtime_error("cannot query times in the future");
    }

    const auto last_time = value > time_t::min()  //
                               ? value - delay_t::epsilon()
                               : value;
    const auto index = find_index(last_time) + 1;
    return HistoryIterator {*this, history_index_t {index}};
}

auto HistoryView::value(time_t value) const -> bool {
    if (value > simulation_time_) [[unlikely]] {
        throw std::runtime_error("cannot query times in the future");
    }
    const auto index = find_index(value);
    return get_value(index);
}

auto HistoryView::last_value() const -> bool {
    return last_value_;
}

auto HistoryView::get_value(std::size_t history_index) const -> bool {
    if (history_ == nullptr) {
        if (history_index != 0) [[unlikely]] {
            throw std::runtime_error("invalid history index");
        }
        return false;
    }

    auto number = history_->size() - history_index;
    return static_cast<bool>(number % 2) ^ last_value_;
}

// Returns the index to the first element that is greater to the value,
// or the history.size() if no such element is found.
auto HistoryView::find_index(time_t value) const -> std::size_t {
    if (history_ == nullptr) {
        return 0;
    }

    const auto it =
        std::ranges::lower_bound(history_->begin() + std::size_t {min_index_},
                                 history_->end(), value, std::ranges::less_equal {});
    const auto index = it - history_->begin();

    // TODO !!! REMOVE CASTS

    assert(index >= static_cast<std::ptrdiff_t>(std::size_t {min_index_}));
    assert(index <= std::ssize(*history_));
    assert(index == std::ssize(*history_) || history_->at(index) > value);
    assert(index == static_cast<std::ptrdiff_t>(std::size_t {min_index_}) ||
           history_->at(index - 1) <= value);

    return gsl::narrow_cast<std::size_t>(index);
}

auto HistoryView::get_time(std::ptrdiff_t index) const -> time_t {
    if (history_ == nullptr) {
        return index < 0 ? time_t::min() : simulation_time_;
    }

    if (index < static_cast<std::ptrdiff_t>(std::size_t {min_index_})) {
        return time_t::min();
    }
    if (index >= std::ssize(*history_)) {
        return simulation_time_;
    }
    return history_->at(index);
}
}  // namespace simulation

}  // namespace logicsim
