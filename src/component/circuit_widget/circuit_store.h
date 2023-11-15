#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_CIRCUIT_STORE_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_CIRCUIT_STORE_H

#include "editable_circuit.h"
#include "interactive_simulation.h"
#include "vocabulary/circuit_widget_state.h"

namespace logicsim {

namespace circuit_widget {

/**
 * @brief: Manages the circuit and their access.
 *
 * Class invariant:
 *     + InteractiveSimulation is only non-empty for SimulationState
 */
class CircuitStore {
   public:
    auto set_circuit_state(CircuitWidgetState new_state) -> void;
    auto set_simulation_config(SimulationConfig new_config) -> void;
    auto set_layout(Layout &&layout) -> void;

    /**
     * @brief: Gives access to the stored layout.
     */
    [[nodiscard]] auto layout() const -> const Layout &;

    /**
     * @brief: Gives possible access to editable_circuit depending on state.
     */
    [[nodiscard]] auto editable_circuit_pointer() const -> const EditableCircuit *;
    [[nodiscard]] auto editable_circuit() -> EditableCircuit &;
    [[nodiscard]] auto editable_circuit() const -> const EditableCircuit &;

    /**
     * @brief: Gives possible access depending on state.
     */
    [[nodiscard]] auto spatial_simulation_pointer() const -> const SpatialSimulation *;
    [[nodiscard]] auto interactive_simulation() -> InteractiveSimulation &;
    [[nodiscard]] auto interactive_simulation() const -> const InteractiveSimulation &;

    /**
     * @brief: Returns statistics if in the simulations state, otherwise nullopt.
     */
    [[nodiscard]] auto simulation_events_per_second() const -> std::optional<double>;

   private:
    CircuitWidgetState circuit_state_ {NonInteractiveState {}};
    SimulationConfig simulation_config_ {};

    EditableCircuit editable_circuit_ {};
    InteractiveSimulation interactive_simulation_ {};
};

}  // namespace circuit_widget

}  // namespace logicsim

#endif
