#include "component/circuit_widget/checked_editable_circuit.h"

#include <gsl/gsl>

#include <stdexcept>

namespace logicsim {

namespace circuit_widget {

namespace {

auto selection_valid(const EditableCircuit& editable_circuit,
                     const CircuitWidgetState& state) -> bool {
    return is_editing_state(state) || (editable_circuit.visible_selection_empty() &&
                                       editable_circuit.selection_count() == 0);
}

}  // namespace

auto CheckedEditableCircuit::set_circuit_state(CircuitWidgetState new_state) -> void {
    Expects(selection_valid(editable_circuit_, circuit_state_));

    circuit_state_ = new_state;

    Ensures(selection_valid(editable_circuit_, new_state));
}

auto CheckedEditableCircuit::circuit_state() const -> CircuitWidgetState {
    Expects(selection_valid(editable_circuit_, circuit_state_));

    return circuit_state_;
}

auto CheckedEditableCircuit::editable_circuit() -> EditableCircuit& {
    Expects(selection_valid(editable_circuit_, circuit_state_));

    return editable_circuit_;
}

auto CheckedEditableCircuit::editable_circuit() const -> const EditableCircuit& {
    Expects(selection_valid(editable_circuit_, circuit_state_));

    return editable_circuit_;
}

auto CheckedEditableCircuit::extract_editable_circuit() -> EditableCircuit {
    Expects(selection_valid(editable_circuit_, circuit_state_));

    const auto result = std::move(editable_circuit_);
    editable_circuit_ = EditableCircuit {Layout {}, result.config()};

    Ensures(selection_valid(editable_circuit_, circuit_state_));
    return result;
}

auto CheckedEditableCircuit::set_editable_circuit(EditableCircuit&& editable_circuit)
    -> void {
    if (!selection_valid(editable_circuit, circuit_state_)) {
        throw std::runtime_error("given editable circuit has wrong amount of selections");
    }

    editable_circuit_ = std::move(editable_circuit);

    Ensures(selection_valid(editable_circuit_, circuit_state_));
}

}  // namespace circuit_widget

}  // namespace logicsim
