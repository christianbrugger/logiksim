#include "core/component/circuit_ui_model/checked_editable_circuit.h"

#include <gsl/gsl>

#include <stdexcept>

namespace logicsim {

namespace circuit_ui_model {

auto CheckedEditableCircuit::set_circuit_state(CircuitWidgetState new_state) -> void {
    Expects(class_invariant_holds());

    circuit_state_ = new_state;

    Ensures(class_invariant_holds());
}

auto CheckedEditableCircuit::circuit_state() const -> CircuitWidgetState {
    Expects(class_invariant_holds());

    return circuit_state_;
}

auto CheckedEditableCircuit::editable_circuit() -> EditableCircuit& {
    Expects(class_invariant_holds());

    return editable_circuit_;
}

auto CheckedEditableCircuit::editable_circuit() const -> const EditableCircuit& {
    Expects(class_invariant_holds());

    return editable_circuit_;
}

auto CheckedEditableCircuit::extract_editable_circuit() -> EditableCircuit {
    Expects(class_invariant_holds());

    const auto result = std::move(editable_circuit_);
    editable_circuit_ = EditableCircuit {Layout {}, get_config(result)};

    Ensures(class_invariant_holds());
    return result;
}

namespace {

[[nodiscard]] auto selection_valid(const EditableCircuit& editable_circuit,
                                   CircuitWidgetState circuit_state) -> bool {
    if (!is_editing_state(circuit_state) &&
        (!editable_circuit.visible_selection_empty() ||
         editable_circuit.visible_selection_operation_count() != 0 ||
         editable_circuit.selection_count() != 0)) {
        return false;
    }

    if (editable_circuit.visible_selection_operation_count() >= 2) {
        return false;
    }

    return true;
}

[[nodiscard]] auto redo_entries_grouped(const EditableCircuit& editable_circuit,
                                        CircuitWidgetState circuit_state) -> bool {
    return is_editing_state(circuit_state) ||
           !has_ungrouped_undo_entries(editable_circuit);
}

}  // namespace

auto CheckedEditableCircuit::set_editable_circuit(EditableCircuit&& editable_circuit)
    -> void {
    if (!selection_valid(editable_circuit, circuit_state_)) {
        throw std::runtime_error("given editable circuit has wrong amount of selections");
    }
    if (!redo_entries_grouped(editable_circuit, circuit_state_)) {
        throw std::runtime_error("given editable circuit has ungrouped redo entries");
    }

    editable_circuit_ = std::move(editable_circuit);
    editable_circuit_.enable_history();

    Ensures(class_invariant_holds());
}

auto CheckedEditableCircuit::class_invariant_holds() const -> bool {
    Expects(selection_valid(editable_circuit_, circuit_state_));
    Expects(redo_entries_grouped(editable_circuit_, circuit_state_));

    return true;
}

}  // namespace circuit_ui_model

}  // namespace logicsim
