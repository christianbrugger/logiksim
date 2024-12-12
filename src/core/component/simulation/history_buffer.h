#ifndef LOGICSIM_COMPONENT_SIMULATION_HISTORY_BUFFER_H
#define LOGICSIM_COMPONENT_SIMULATION_HISTORY_BUFFER_H

#include "core/container/circular_buffer.h"
#include "core/format/container.h"
#include "core/vocabulary/time.h"

#include <compare>
#include <initializer_list>

namespace logicsim {

namespace simulation {

struct history_index_t;

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
    [[nodiscard]] HistoryBuffer(std::initializer_list<value_type> list);

    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto ssize() const -> std::ptrdiff_t;

    auto pop_front() -> void;
    auto push_back(time_t transition_time) -> void;

    [[nodiscard]] auto front() const -> const time_t &;
    [[nodiscard]] auto back() const -> const time_t &;

    [[nodiscard]] auto begin() const -> const_iterator;
    [[nodiscard]] auto end() const -> const_iterator;

    /**
     * brief: range checked lookup of transition time at given index.
     */
    [[nodiscard]] auto at(history_index_t index) const -> const time_t &;

   private:
    container_t buffer_ {};
};

static_assert(sizeof(HistoryBuffer) == 32);

}  // namespace simulation

}  // namespace logicsim

#endif
