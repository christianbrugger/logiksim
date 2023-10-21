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
    min_index_ = find_index(first_time);

    assert(min_index_ >= history_index_t {0});
    assert(size() >= 1);
}

auto HistoryView::size() const -> std::size_t {
    if (history_ == nullptr) {
        return 1;
    }
    assert(min_index_ >= history_index_t {0});
    assert(history_->ssize() > std::ptrdiff_t {min_index_});

    return history_->size() + 1 - static_cast<std::size_t>(std::ptrdiff_t {min_index_});
}

auto HistoryView::ssize() const -> std::ptrdiff_t {
    return static_cast<std::ptrdiff_t>(size());
}

auto HistoryView::begin() const -> HistoryIterator {
    return HistoryIterator {*this, min_index_};
}

auto HistoryView::end() const -> HistoryIterator {
    return HistoryIterator {*this, size() + min_index_};
}

auto HistoryView::from(time_t value) const -> HistoryIterator {
    if (value > simulation_time_) [[unlikely]] {
        throw std::runtime_error("cannot query times in the future");
    }
    const auto index = find_index(value);
    return HistoryIterator {*this, index};
}

auto HistoryView::until(time_t value) const -> HistoryIterator {
    if (value > simulation_time_) [[unlikely]] {
        throw std::runtime_error("cannot query times in the future");
    }

    const auto last_time = value > time_t::min()  //
                               ? value - delay_t::epsilon()
                               : value;
    const auto index = find_index(last_time) + 1;
    return HistoryIterator {*this, index};
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

auto HistoryView::get_value(history_index_t history_index) const -> bool {
    if (history_ == nullptr) {
        if (history_index != history_index_t {0}) [[unlikely]] {
            throw std::runtime_error("invalid history index");
        }
        return false;
    }

    auto number = std::ssize(*history_) - std::ptrdiff_t {history_index};
    // makes modulo work with negative numbers
    if (number < std::ptrdiff_t {0}) {
        number += std::numeric_limits<std::ptrdiff_t>::min();
    }
    return static_cast<bool>(number % 2) ^ last_value_;
}

// Returns the index to the first element that is greater to the value,
// or the history.size() if no such element is found.
auto HistoryView::find_index(time_t value) const -> history_index_t {
    if (history_ == nullptr) {
        return history_index_t {0};
    }
    assert(min_index_ >= history_index_t {0});

    const auto it =
        std::ranges::lower_bound(history_->begin() + std::ptrdiff_t {min_index_},
                                 history_->end(), value, std::ranges::less_equal {});
    const auto index = it - history_->begin();

    assert(index >= 0);
    assert(index >= std::ptrdiff_t {min_index_});
    assert(index <= std::ssize(*history_));
    assert(index == std::ssize(*history_) ||
           history_->at(history_index_t {index}) > value);
    assert(index == std::ptrdiff_t {min_index_} ||
           history_->at(history_index_t {index - 1}) <= value);

    return history_index_t {index};
}

auto HistoryView::get_time(history_index_t index) const -> time_t {
    if (history_ == nullptr) {
        return index < history_index_t {0} ? time_t::min() : simulation_time_;
    }

    if (index < min_index_) {
        return time_t::min();
    }
    if (std::ptrdiff_t {index} >= std::ssize(*history_)) {
        return simulation_time_;
    }
    return history_->at(index);
}
}  // namespace simulation

}  // namespace logicsim
