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
 *     + EditableCircuit is only changed for EditingState
 */
class CircuitStore {
   public:
    auto set_circuit_state(CircuitWidgetState new_state) -> void;
    auto set_simulation_config(SimulationConfig new_config) -> void;

    /**
     * @brief: Gives access to the stored layout.
     */
    [[nodiscard]] auto layout() const -> const Layout &;

    /**
     * @brief: Gives possible editing access depending on state.
     *
     * Returns nullptr if state is not EditingState.
     */
    [[nodiscard]] auto editable_circuit() -> EditableCircuit *;
    [[nodiscard]] auto editable_circuit() const -> const EditableCircuit *;

    /**
     * @brief: Gives possible simulation access depending on state.
     *
     * Returns nullptr if state is not SimulationState.
     */
    [[nodiscard]] auto interactive_simulation() -> InteractiveSimulation *;
    [[nodiscard]] auto interactive_simulation() const -> const InteractiveSimulation *;
    [[nodiscard]] auto spatial_simulation() const -> const SpatialSimulation *;

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
