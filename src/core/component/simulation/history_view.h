#ifndef LOGICSIM_COMPONENT_SIMULATION_HISTORY_VIEW_H
#define LOGICSIM_COMPONENT_SIMULATION_HISTORY_VIEW_H

#include "core/component/simulation/history_calculation.h"
#include "core/component/simulation/history_iterator.h"

#include <cstddef>
#include <ranges>

namespace logicsim {

struct time_t;
struct delay_t;

namespace simulation {

class HistoryIterator;
class HistoryBuffer;
struct history_entry_t;

/**
 * @brief: View over History that allows iterating.
 */
class HistoryView {
   public:
    using iterator_type = HistoryIterator;

    using value_type = history_entry_t;
    using pointer = history_entry_t;
    using reference = history_entry_t;

   public:
    [[nodiscard]] explicit HistoryView() = default;
    [[nodiscard]] explicit HistoryView(const HistoryBuffer &history,
                                       time_t simulation_time, bool last_value,
                                       delay_t history_length);

    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto ssize() const -> std::ptrdiff_t;

    [[nodiscard]] auto begin() const -> HistoryIterator;
    [[nodiscard]] auto end() const -> HistoryIterator;

    [[nodiscard]] auto from(time_t value) const -> HistoryIterator;
    [[nodiscard]] auto until(time_t value) const -> HistoryIterator;

    [[nodiscard]] auto value(time_t value) const -> bool;
    [[nodiscard]] auto last_value() const -> bool;
    [[nodiscard]] auto simulation_time() const -> time_t;

   private:
    HistoryCalculationData data_;
};

static_assert(std::ranges::forward_range<HistoryView>);

}  // namespace simulation

}  // namespace logicsim

template <>
inline constexpr bool std::ranges::enable_view<logicsim::simulation::HistoryView> = true;

#endif
