#ifndef LOGICSIM_COMPONENT_SIMULATION_HISTORY_CALCULATION_H
#define LOGICSIM_COMPONENT_SIMULATION_HISTORY_CALCULATION_H

#include "component/simulation/history_index.h"
#include "component/simulation/history_min_index.h"
#include "vocabulary/time.h"

namespace logicsim {

struct delay_t;

namespace simulation {

class HistoryBuffer;

/**
 * brief: Returns extrapolated time at index.
 *
 * The the result is clamped.
 *      For index < 0 time_t::min() is returned.
 *      For index >= size(history) simulation_time is returned.
 *      For history == nullptr a size of 0 is assumed.
 */
[[nodiscard]] auto get_time_extrapolated(const HistoryBuffer *history,
                                         history_index_t history_index,
                                         history_min_index_t min_index,
                                         time_t simulation_time) -> time_t;

/**
 * brief: Returns extrapolated value time.
 */
[[nodiscard]] auto get_value_extrapolated(const HistoryBuffer *history,
                                          history_index_t history_index, bool last_value)
    -> bool;

/**
 * brief: Returns history index of given time.
 */
[[nodiscard]] auto find_index_extrapolated(const HistoryBuffer *history, time_t value,
                                           history_min_index_t min_index)
    -> history_index_t;

[[nodiscard]] auto calculate_min_index(const HistoryBuffer *history,
                                       time_t simulation_time, delay_t history_length)
    -> history_min_index_t;

//
// History Calculation Data
//

/**
 * @brief: All data used for history calculations.
 */
struct HistoryCalculationData {
    const HistoryBuffer *history {nullptr};
    time_t simulation_time {time_t::max()};
    history_min_index_t min_index {history_index_t {0}};
    bool last_value {false};
};

[[nodiscard]] auto get_time_extrapolated(const HistoryCalculationData &data,
                                         history_index_t history_index) -> time_t;

[[nodiscard]] auto get_value_extrapolated(const HistoryCalculationData &data,
                                          history_index_t history_index) -> bool;

[[nodiscard]] auto find_index_extrapolated(const HistoryCalculationData &data,
                                           time_t value) -> history_index_t;

}  // namespace simulation

}  // namespace logicsim

#endif
