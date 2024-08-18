#ifndef LOGICSIM_COMPONENT_SIMULATION_SIMULATION_EVENT_H
#define LOGICSIM_COMPONENT_SIMULATION_SIMULATION_EVENT_H

#include "format/struct.h"
#include "vocabulary/connection.h"
#include "vocabulary/element_id.h"
#include "vocabulary/time.h"

#include <compare>

namespace logicsim {

namespace simulation {

/**
 * @brief: Future logic value transition at a specific time and logic item input.
 */
struct simulation_event_t {
    time_t time;
    element_id_t element_id;
    connection_id_t input_id;
    bool value;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const simulation_event_t &) const -> bool = default;
    [[nodiscard]] auto operator<=>(const simulation_event_t &) const = default;
};

static_assert(sizeof(simulation_event_t) == 16);

static_assert(std::is_aggregate_v<simulation_event_t>);
static_assert(std::is_trivially_copyable_v<simulation_event_t>);
static_assert(std::is_trivially_copy_constructible_v<simulation_event_t>);
static_assert(std::is_trivially_copy_assignable_v<simulation_event_t>);

/**
 * @brief: Check if time or element_id is greater for two events.
 */
struct greater_time_element_id {
    [[nodiscard]] auto operator()(const simulation_event_t &left,
                                  const simulation_event_t &right) const -> bool;
};

}  // namespace simulation

}  // namespace logicsim

#endif
