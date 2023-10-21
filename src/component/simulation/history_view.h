#ifndef LOGICSIM_COMPONENT_SIMULATION_HISTORY_VIEW_H
#define LOGICSIM_COMPONENT_SIMULATION_HISTORY_VIEW_H

#include "component/simulation/history_buffer.h"
#include "component/simulation/history_entry.h"
#include "component/simulation/history_index.h"
#include "vocabulary/time.h"

#include <cstddef>
#include <ranges>

namespace logicsim {

struct delay_t;

namespace simulation {

class HistoryIterator;

class HistoryView {
    // TODO remove circular dependency
    friend HistoryIterator;

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

   private:
    [[nodiscard]] auto get_value(std::size_t history_index) const -> bool;
    [[nodiscard]] auto find_index(time_t value) const -> std::size_t;
    [[nodiscard]] auto get_time(std::ptrdiff_t index) const -> time_t;

   private:
    const HistoryBuffer *history_ {nullptr};
    time_t simulation_time_ {time_t::max()};
    history_index_t min_index_ {};
    bool last_value_ {};
};

}  // namespace simulation

}  // namespace logicsim

template <>
inline constexpr bool std::ranges::enable_view<logicsim::simulation::HistoryView> = true;

#endif
