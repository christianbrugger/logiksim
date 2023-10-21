#ifndef LOGICSIM_COMPONENT_SIMULATION_SIMULATION_EVENT_H
#define LOGICSIM_COMPONENT_SIMULATION_SIMULATION_EVENT_H

#include "format/struct.h"
#include "schematic_old.h"  // TODO remove
#include "vocabulary/connection_id.h"
#include "vocabulary/element_id.h"
#include "vocabulary/time.h"

namespace logicsim {

namespace simulation {

// TODO rename to simulation_event_t
struct SimulationEvent {
    time_t time;
    // TODO replace with input_t
    element_id_t element_id;
    connection_id_t input_index;
    bool value;

    auto operator==(const SimulationEvent &other) const -> bool;
    auto operator<(const SimulationEvent &other) const -> bool;

    // TODO replace with <=>
    auto operator!=(const SimulationEvent &other) const -> bool;
    auto operator>(const SimulationEvent &other) const -> bool;
    auto operator<=(const SimulationEvent &other) const -> bool;
    auto operator>=(const SimulationEvent &other) const -> bool;

    [[nodiscard]] auto format() const -> std::string;
};

static_assert(sizeof(SimulationEvent) == 16);

static_assert(std::is_standard_layout_v<SimulationEvent>);
static_assert(std::is_trivially_copyable_v<SimulationEvent>);
static_assert(std::is_trivially_copy_constructible_v<SimulationEvent>);
static_assert(std::is_trivially_copy_assignable_v<SimulationEvent>);

// TODO remove SchematicOld
auto make_event(SchematicOld::ConstInput input, time_t time, bool value)
    -> SimulationEvent;

}  // namespace simulation

}  // namespace logicsim

#endif
