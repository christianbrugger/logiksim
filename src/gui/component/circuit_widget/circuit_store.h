#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_CIRCUIT_STORE_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_CIRCUIT_STORE_H

#include "gui/component/circuit_widget/checked_editable_circuit.h"

#include "core/editable_circuit.h"
#include "core/format/struct.h"
#include "core/interactive_simulation.h"
#include "core/vocabulary/circuit_widget_state.h"
#include "core/vocabulary/simulation_config.h"

#include <optional>

namespace logicsim {

struct CircuitStoreAllocInfo;

namespace circuit_widget {

/**
 * @brief: Manages the circuit and creates the simulation as needed.
 *
 * Pre-condition:
 *    + No reference to layout, editable-circuit, or interactive-simulation
 *      are stored outside across multiple calls.
 *
 * Class invariant:
 *     + InteractiveSimulation is not-null in simulation state and null otherwise.
 *     + Layout is the same for Simulation and EditableCircuit in simulation state.
 *     + InteractiveSimulation delay is the same as simulation config.
 *     + Circuit state is the same as in checked_editable_circuit.
 */
class CircuitStore {
   public:
    auto set_circuit_state(CircuitWidgetState new_state) -> void;
    auto set_simulation_config(SimulationConfig new_config) -> void;
    [[nodiscard]] auto circuit_state() const -> CircuitWidgetState;
    [[nodiscard]] auto simulation_config() const -> SimulationConfig;
    [[nodiscard]] auto allocation_info() const -> CircuitStoreAllocInfo;

    /**
     * @brief: Set a new editable circuit in any state.
     *
     * Also sets the new simulations config if provided.
     *
     * Note, This methods regenerates the active simulation if present.
     */
    auto set_editable_circuit(EditableCircuit &&editable_circuit) -> void;

    /**
     * @brief: Gives access to the stored layout. This is always available.
     *
     * Useful especially for non-interactive states.
     *
     * Pre-condition:
     *   + No reference to layout is stored outside across multiple calls.
     */
    [[nodiscard]] auto layout() const -> const Layout &;

    /**
     * @brief: Gives access to editable_circuit in editing state.
     *
     * Throws exception if not in editing state.
     *
     * Pre-condition:
     *   + No reference to editable circuit is stored outside across multiple calls.
     */
    [[nodiscard]] auto editable_circuit() -> EditableCircuit &;
    [[nodiscard]] auto editable_circuit() const -> const EditableCircuit &;
    [[nodiscard]] auto extract_editable_circuit() -> EditableCircuit;

    /**
     * @brief: Gives access to simulation in simulation.
     *
     * Throws exception if not in simulation state.
     *
     * Pre-condition:
     *   + No reference to interactive simulation is stored outside across multiple calls.
     */
    [[nodiscard]] auto interactive_simulation() -> InteractiveSimulation &;
    [[nodiscard]] auto interactive_simulation() const -> const InteractiveSimulation &;

    /**
     * @brief: Returns statistics if in the simulations state, otherwise nullopt.
     */
    [[nodiscard]] auto simulation_events_per_second() const -> std::optional<double>;

   private:
    [[nodiscard]] auto class_invariant_holds() const -> bool;

   private:
    CircuitWidgetState circuit_state_ {NonInteractiveState {}};
    SimulationConfig simulation_config_ {};

    CheckedEditableCircuit checked_editable_circuit_ {};
    std::optional<InteractiveSimulation> interactive_simulation_ {};
};

//
// Free functions
//

[[nodiscard]] auto editable_circuit_pointer(CircuitStore &store) -> EditableCircuit *;

[[nodiscard]] auto editable_circuit_pointer(const CircuitStore &store)
    -> const EditableCircuit *;

[[nodiscard]] auto spatial_simulation_pointer(const CircuitStore &store)
    -> const SpatialSimulation *;

[[nodiscard]] auto visible_selection_format(const CircuitStore &store) -> std::string;

}  // namespace circuit_widget

}  // namespace logicsim

#endif
