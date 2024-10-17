#ifndef LOGICSIM_COMPONENT_SIMULATION_SIMULATION_EVENT_GROUP_H
#define LOGICSIM_COMPONENT_SIMULATION_SIMULATION_EVENT_GROUP_H

#include "core/component/simulation/simulation_event.h"
#include "core/format/container.h"

#include <folly/small_vector.h>

namespace logicsim {

namespace simulation {

/**
 * @brief: groups of events for the same element and time but different inputs.
 *
 * Class invariants:
 *     * All events need to have same time
 *     * All events need to have same element_id
 *     * No two event.input_id are equal
 */
class SimulationEventGroup {
   public:
    using value_type = simulation_event_t;
    using container_t = folly::small_vector<value_type, 4>;

    using iterator = container_t::const_iterator;
    using const_iterator = container_t::const_iterator;

   public:
    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;

    auto push_back(simulation_event_t event) -> void;

    [[nodiscard]] auto front() const -> const simulation_event_t &;
    [[nodiscard]] auto back() const -> const simulation_event_t &;

    [[nodiscard]] auto begin() const -> const_iterator;
    [[nodiscard]] auto end() const -> const_iterator;

   private:
    container_t group_ {};
};

}  // namespace simulation

}  // namespace logicsim

#endif
