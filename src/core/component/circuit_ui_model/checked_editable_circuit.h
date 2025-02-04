#ifndef LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_CHECKED_EDITABLE_CIRCUIT_H
#define LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_CHECKED_EDITABLE_CIRCUIT_H

#include "core/editable_circuit.h"
#include "core/vocabulary/circuit_widget_state.h"

namespace logicsim {

namespace circuit_ui_model {

/**
 * @brief: CheckedEditableCircuit with checked number of selections.
 *
 * Pre-condition:
 *   + No reference to editable circuit is stored outside across multiple calls.
 *
 * Class-invariant:
 *   + editable-circuit has no selection in non-editing-states
 *   + editable-circuit has no visible selection in non-editing-states
 *   + number of visible-selection operations is maximum 1 (for optimization)
 *   + editable-circuit has no ungrouped redo entries in non-editing states
 *   + editable-circuit history is always enabled
 */
class CheckedEditableCircuit {
   public:
    auto set_circuit_state(CircuitWidgetState new_state) -> void;
    [[nodiscard]] auto circuit_state() const -> CircuitWidgetState;

    [[nodiscard]] auto editable_circuit() -> EditableCircuit &;
    [[nodiscard]] auto editable_circuit() const -> const EditableCircuit &;
    [[nodiscard]] auto extract_editable_circuit() -> EditableCircuit;

    auto set_editable_circuit(EditableCircuit &&editable_circuit) -> void;

   private:
    [[nodiscard]] auto class_invariant_holds() const -> bool;

   private:
    CircuitWidgetState circuit_state_ {NonInteractiveState {}};
    EditableCircuit editable_circuit_ {Layout {},
                                       EditableCircuit::Config {.enable_history = true}};
};

}  // namespace circuit_ui_model

}  // namespace logicsim

#endif
