#include "core/component/simulation/history_view.h"

#include "core/component/simulation/history_buffer.h"
#include "core/component/simulation/history_iterator.h"
#include "core/vocabulary/delay.h"

#include <gsl/gsl>

#include <algorithm>
#include <stdexcept>

namespace logicsim {

namespace simulation {

HistoryView::HistoryView(const HistoryBuffer &history, time_t simulation_time,
                         bool last_value, delay_t history_length)
    : data_ {{
          .history = &history,
          .simulation_time = simulation_time,
          .min_index = calculate_min_index(&history, simulation_time, history_length),
          .last_value = last_value,
      }} {
    assert(size() >= 1);
}

auto HistoryView::size() const -> std::size_t {
    if (data_.history == nullptr) {
        return 1;
    }
    assert(data_.history->size() + 1 >= std::size_t {data_.min_index});

    return data_.history->size() + 1 - std::size_t {data_.min_index};
}

auto HistoryView::ssize() const -> std::ptrdiff_t {
    return static_cast<std::ptrdiff_t>(size());
}

auto HistoryView::begin() const -> HistoryIterator {
    return HistoryIterator {data_, data_.min_index.index};
}

auto HistoryView::end() const -> HistoryIterator {
    return HistoryIterator {data_, data_.min_index.index + size()};
}

auto HistoryView::from(time_t value) const -> HistoryIterator {
    if (value > data_.simulation_time) [[unlikely]] {
        throw std::runtime_error("cannot query times in the future");
    }

    const auto index = find_index_extrapolated(data_, value);
    return HistoryIterator {data_, index};
}

auto HistoryView::until(time_t value) const -> HistoryIterator {
    if (value > data_.simulation_time) [[unlikely]] {
        throw std::runtime_error("cannot query times in the future");
    }

    const auto last_time = value > time_t::min() ? value - delay_t::epsilon() : value;
    const auto index = find_index_extrapolated(data_, last_time) + 1;

    return HistoryIterator {data_, index};
}

auto HistoryView::value(time_t value) const -> bool {
    if (value > data_.simulation_time) [[unlikely]] {
        throw std::runtime_error("cannot query times in the future");
    }

    const auto index = find_index_extrapolated(data_, value);
    return get_value_extrapolated(data_, index);
}

auto HistoryView::last_value() const -> bool {
    return data_.last_value;
}

auto HistoryView::simulation_time() const -> time_t {
    return data_.simulation_time;
}

}  // namespace simulation

}  // namespace logicsim
