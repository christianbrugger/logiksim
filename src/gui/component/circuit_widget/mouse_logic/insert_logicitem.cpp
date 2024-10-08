#include "component/circuit_widget/mouse_logic/insert_logicitem.h"

#include "editable_circuit.h"
#include "vocabulary/insertion_mode.h"

#include <optional>

namespace logicsim {

namespace circuit_widget {

namespace {

auto remove_and_insert(EditableCircuit& editable_circuit, selection_id_t selection_id,
                       const LogicItemDefinition& element_definition,
                       std::optional<point_t> position,
                       InsertionMode mode) -> selection_id_t {
    save_delete_all(editable_circuit, selection_id);

    if (!editable_circuit.selection_exists(selection_id)) {
        selection_id = editable_circuit.create_selection();
    }

    if (position) {
        editable_circuit.add_logicitem(element_definition, position.value(), mode,
                                       selection_id);
    }

    return selection_id;
}

}  // namespace

InsertLogicItemLogic::InsertLogicItemLogic(LogicItemDefinition element_definition)
    : element_definition_ {std::move(element_definition)} {}

auto InsertLogicItemLogic::mouse_press(EditableCircuit& editable_circuit,
                                       std::optional<point_t> position) -> void {
    temp_element_ =
        remove_and_insert(editable_circuit, temp_element_, element_definition_, position,
                          InsertionMode::collisions);
}

auto InsertLogicItemLogic::mouse_move(EditableCircuit& editable_circuit,
                                      std::optional<point_t> position) -> void {
    temp_element_ =
        remove_and_insert(editable_circuit, temp_element_, element_definition_, position,
                          InsertionMode::collisions);
}

auto InsertLogicItemLogic::mouse_release(EditableCircuit& editable_circuit,
                                         std::optional<point_t> position) -> void {
    temp_element_ =
        remove_and_insert(editable_circuit, temp_element_, element_definition_, position,
                          InsertionMode::insert_or_discard);

    save_destroy_selection(editable_circuit, temp_element_);
    temp_element_ = null_selection_id;
}

auto InsertLogicItemLogic::finalize(EditableCircuit& editable_circuit) -> void {
    save_delete_all(editable_circuit, temp_element_);
    save_destroy_selection(editable_circuit, temp_element_);
    temp_element_ = null_selection_id;
}

}  // namespace circuit_widget

}  // namespace logicsim
