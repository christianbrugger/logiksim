#include "editable_circuit2.h"

namespace logicsim {

EditableCircuit2::EditableCircuit2(Layout&& layout__) : modifier_ {std::move(layout__)} {}

auto EditableCircuit2::format() const -> std::string {
    return modifier_.format();
}

auto EditableCircuit2::add_logic_item(const LogicItemDefinition& definition,
                                      point_t position, InsertionMode insertion_mode,
                                      selection_id_t selection_id) -> void {
    const auto logicitem_id =
        modifier_.add_logic_item(definition, position, insertion_mode);

    if (logicitem_id) {
        modifier_.add_to_selection(selection_id, logicitem_id);
    }
}

auto EditableCircuit2::add_wire_segments(point_t p0, point_t p1,
                                         LineInsertionType segment_type,
                                         InsertionMode insertion_mode,
                                         selection_id_t selection_id) -> void {
    editable_circuit::add_wire_segments(modifier_, p0, p1, segment_type, insertion_mode,
                                        selection_id);
}

auto EditableCircuit2::change_insertion_mode(selection_id_t selection_id,
                                             InsertionMode new_insertion_mode) -> void {
    using namespace editable_circuit;

    const auto guard = ModifierSelectionGuard(modifier_, selection_id);
    change_insertion_mode_consuming(modifier_, guard.selection_id(), new_insertion_mode);
}

auto EditableCircuit2::change_insertion_mode(Selection selection__,
                                             InsertionMode new_insertion_mode) -> void {
    using namespace editable_circuit;

    const auto guard = ModifierSelectionGuard(modifier_, std::move(selection__));
    change_insertion_mode_consuming(modifier_, guard.selection_id(), new_insertion_mode);
}

auto EditableCircuit2::move_or_delete_temporary(selection_id_t selection_id, int delta_x,
                                                int delta_y) -> void {
    using namespace editable_circuit;

    const auto guard = ModifierSelectionGuard(modifier_, selection_id);
    move_or_delete_temporary_consuming(modifier_, guard.selection_id(), delta_x, delta_y);
}

auto EditableCircuit2::move_or_delete_temporary(Selection selection__, int delta_x,
                                                int delta_y) -> void {
    using namespace editable_circuit;

    const auto guard = ModifierSelectionGuard(modifier_, std::move(selection__));
    move_or_delete_temporary_consuming(modifier_, guard.selection_id(), delta_x, delta_y);
}

auto EditableCircuit2::move_temporary_unchecked(const Selection& selection, int delta_x,
                                                int delta_y) -> void {
    editable_circuit::move_temporary_unchecked(modifier_, selection, delta_x, delta_y);
}

auto EditableCircuit2::delete_all(selection_id_t selection_id) -> void {
    editable_circuit::delete_all(modifier_, selection_id);
}

auto EditableCircuit2::delete_all(Selection selection__) -> void {
    using namespace editable_circuit;

    const auto guard = ModifierSelectionGuard(modifier_, std::move(selection__));
    editable_circuit::delete_all(modifier_, guard.selection_id());
}

//
// Selections
//

auto EditableCircuit2::create_selection() -> selection_id_t {
    return modifier_.create_selection();
}

}  // namespace logicsim
