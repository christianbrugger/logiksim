#ifndef LOGICSIM_COMPONENT_SIMULATION_HISTORY_BUFFER_H
#define LOGICSIM_COMPONENT_SIMULATION_HISTORY_BUFFER_H

#include "container/circular_buffer.h"
#include "format/container.h"
#include "vocabulary/time.h"

#include <compare>
#include <initializer_list>

namespace logicsim {

namespace simulation {

/**
 * @brief: Store the history of a logic input by storing transition times.
 *
 * Class invariants:
 *     * times in buffer are sorted strictly ascending
 */
class HistoryBuffer {
   public:
    using value_type = time_t;
    using container_t = circular_buffer<time_t, 2, uint32_t>;

    using iterator = container_t::const_iterator;
    using const_iterator = container_t::const_iterator;

   public:
    [[nodiscard]] explicit HistoryBuffer() = default;
    [[nodiscard]] explicit HistoryBuffer(std::initializer_list<value_type> list);

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;

    auto pop_front() -> void;
    auto push_back(time_t transition_time) -> void;

    [[nodiscard]] auto at(std::size_t index) const -> const time_t &;
    [[nodiscard]] auto front() const -> const time_t &;
    [[nodiscard]] auto back() const -> const time_t &;

    [[nodiscard]] auto begin() const -> const_iterator;
    [[nodiscard]] auto end() const -> const_iterator;

   private:
    container_t buffer_ {};
};

static_assert(sizeof(HistoryBuffer) == 28);

}  // namespace simulation

}  // namespace logicsim

#endif
