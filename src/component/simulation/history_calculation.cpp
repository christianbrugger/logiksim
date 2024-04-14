#include "component/simulation/history_calculation.h"

#include "component/simulation/history_buffer.h"
#include "component/simulation/history_index.h"
#include "vocabulary/delay.h"

#include <stdexcept>

namespace logicsim {

namespace simulation {

//
// Free Functions
//

auto get_time_extrapolated(const HistoryBuffer* history, history_index_t history_index,
                           history_min_index_t min_index, time_t simulation_time)
    -> time_t {
    if (history == nullptr) {
        return history_index < history_index_t {0} ? time_t::min() : simulation_time;
    }
    if (history_index < min_index) {
        return time_t::min();
    }
    if (std::ptrdiff_t {history_index} >= std::ssize(*history)) {
        return simulation_time;
    }
    return history->at(history_index);
}

auto get_value_extrapolated(const HistoryBuffer* history, history_index_t history_index,
                            bool last_value) -> bool {
    if (history == nullptr) {
        return last_value;
    }

    auto number = std::ssize(*history) - std::ptrdiff_t {history_index};

    // make modulo work with negative numbers
    if (number < std::ptrdiff_t {0}) {
        number += std::numeric_limits<std::ptrdiff_t>::min();
    }

    return static_cast<bool>(number % 2) ^ last_value;
}

auto find_index_extrapolated(const HistoryBuffer* history, time_t value,
                             history_min_index_t min_index) -> history_index_t {
    if (history == nullptr) {
        return history_index_t {0};
    }

    const auto it =
        std::ranges::lower_bound(history->begin() + std::ptrdiff_t {min_index},
                                 history->end(), value, std::ranges::less_equal {});
    const auto index = history_index_t {it - history->begin()};

    assert(index >= min_index);
    assert(std::ptrdiff_t {index} <= std::ssize(*history));
    assert(std::ptrdiff_t {index} == std::ssize(*history) || history->at(index) > value);
    assert(index == min_index || history->at(index - 1) <= value);

    return history_index_t {index};
}

auto calculate_min_index(const HistoryBuffer* history, time_t simulation_time,
                         delay_t history_length) -> history_min_index_t {
    const auto first_time = simulation_time - history_length;
    return history_min_index_t {
        find_index_extrapolated(history, first_time, history_min_index_t {})};
}

//
// History Calculation Data
//

[[nodiscard]] HistoryCalculationData::HistoryCalculationData(New data)
    : history {data.history},
      simulation_time {data.simulation_time},
      min_index {data.min_index},
      last_value {data.last_value} {
    if (history == nullptr) {
        if (min_index != history_min_index_t {}) [[unlikely]] {
            throw std::runtime_error("min index needs to be zero if no history is given");
        }
    } else {
        if (!(history_min_index_t {} <= min_index &&
              std::ptrdiff_t {min_index} <= history->ssize())) [[unlikely]] {
            throw std::runtime_error("min index out of history bounds");
        }
        if (!history->empty() && simulation_time < history->back()) [[unlikely]] {
            throw std::runtime_error("simulation time in the past");
        }
    }
}

auto get_time_extrapolated(const HistoryCalculationData& data,
                           history_index_t history_index) -> time_t {
    return get_time_extrapolated(data.history, history_index, data.min_index,
                                 data.simulation_time);
}

auto get_value_extrapolated(const HistoryCalculationData& data,
                            history_index_t history_index) -> bool {
    return get_value_extrapolated(data.history, history_index, data.last_value);
}

auto find_index_extrapolated(const HistoryCalculationData& data, time_t value)
    -> history_index_t {
    return find_index_extrapolated(data.history, value, data.min_index);
}

}  // namespace simulation

}  // namespace logicsim
