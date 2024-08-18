#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_CHECKED_EDITABLE_CIRCUIT_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_CHECKED_EDITABLE_CIRCUIT_H

#include "editable_circuit.h"
#include "vocabulary/circuit_widget_state.h"

namespace logicsim {

namespace circuit_widget {

/**
 * @brief: CheckedEditableCircuit with checked number of selections.
 *
 * Pre-condition:
 *   + No reference to editable circuit is stored outside across multiple calls.
 *
 * Class-invariant:
 *   + editable-circuit has no selection in non-editing-states
 *   + editable-circuit has no visible selection is non-editing-states
 *   + number of visible-selection operations is maximum 1 (for optimization)
 */
class CheckedEditableCircuit {
   public:
    auto set_circuit_state(CircuitWidgetState new_state) -> void;
    [[nodiscard]] auto circuit_state() const -> CircuitWidgetState;

    [[nodiscard]] auto editable_circuit() -> EditableCircuit &;
    [[nodiscard]] auto editable_circuit() const -> const EditableCircuit &;
    auto set_editable_circuit(EditableCircuit &&editable_circuit) -> void;

   private:
    CircuitWidgetState circuit_state_ {NonInteractiveState {}};
    EditableCircuit editable_circuit_ {};
};

}  // namespace circuit_widget

}  // namespace logicsim

#endif
