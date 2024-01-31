#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_CIRCUIT_STORE_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_CIRCUIT_STORE_H

#include "component/circuit_widget/checked_editable_circuit.h"
#include "editable_circuit.h"
#include "format/struct.h"
#include "interactive_simulation.h"
#include "vocabulary/circuit_widget_state.h"
#include "vocabulary/view_config.h"

#include <optional>

namespace logicsim {

namespace circuit_widget {

namespace circuit_store {

struct LoadFileResult {
    bool success {false};
    ViewPoint view_point {};
    SimulationConfig simulation_config {};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const LoadFileResult &) const -> bool = default;
};

}  // namespace circuit_store

/**
 * @brief: Manages the circuit and creates the simulation as needed.
 *
 * Pre-condition:
 *   + No reference to layout, editable-circuit, or interactive-simulation
 *     are stored outside across multiple calls.
 *
 * Class invariant:
 *     + InteractiveSimulation is not-null in simulation state and null otherwise.
 *     + Layout is the same for Simulation and EditableCircuit in simulation state.
 *     + InteractiveSimulation delay is the same as simulation config.
 *     + Circuit state is the same as in checked_editable_circuit.
 */
class CircuitStore {
   public:
    using LoadFileResult = circuit_store::LoadFileResult;

   public:
    auto set_circuit_state(CircuitWidgetState new_state) -> void;
    auto set_simulation_config(SimulationConfig new_config) -> void;
    [[nodiscard]] auto circuit_state() const -> CircuitWidgetState;
    [[nodiscard]] auto simulation_config() const -> SimulationConfig;

    /**
     * @brief: Set a new editable circuit in any state.
     *
     * Also sets the new simulations config if provided.
     *
     * Note, This methods regenerates the active simulation if present.
     */
    auto set_editable_circuit(EditableCircuit &&editable_circuit,
                              std::optional<SimulationConfig> new_config = {}) -> void;

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

[[nodiscard]] auto editable_circuit_pointer(CircuitStore &store)
    -> EditableCircuit *;

[[nodiscard]] auto editable_circuit_pointer(const CircuitStore &store)
    -> const EditableCircuit *;

[[nodiscard]] auto spatial_simulation_pointer(const CircuitStore &store)
    -> const SpatialSimulation *;

auto set_layout(CircuitStore &store, Layout &&layout,
                std::optional<SimulationConfig> new_config = {}) -> void;

auto load_from_file(CircuitStore &store, std::string filename)
    -> circuit_store::LoadFileResult;

auto load_circuit_example(CircuitStore &store, int number,
                          std::optional<SimulationConfig> new_config = {}) -> void;

auto save_circuit(const CircuitStore &store, std::string filename, ViewPoint view_point)
    -> bool;

// TODO remove once we have history
[[nodiscard]] auto serialize_circuit(const CircuitStore &store) -> std::string;

[[nodiscard]] auto visible_selection_format(const CircuitStore &store) -> std::string;

}  // namespace circuit_widget

}  // namespace logicsim

#endif
