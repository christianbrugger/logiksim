#include "component/circuit_widget/mouse_logic/insert_wire.h"

#include "editable_circuit.h"
#include "geometry/orientation.h"
#include "vocabulary/insertion_mode.h"

namespace logicsim {

namespace circuit_widget {

namespace {

auto remove_and_insert(EditableCircuit& editable_circuit, selection_id_t selection_id,
                       std::optional<point_t> first_position,
                       std::optional<LineInsertionType> direction,
                       std::optional<point_t> position, InsertionMode mode)
    -> selection_id_t {
    save_delete_all(editable_circuit, selection_id);

    if (!editable_circuit.selection_exists(selection_id)) {
        selection_id = editable_circuit.create_selection();
    }

    if (position && first_position && direction && position != first_position) {
        // do insert
        add_wire_segments(editable_circuit, *first_position, *position, *direction,
                          InsertionMode::temporary, selection_id);

        if (mode != InsertionMode::temporary) {
            editable_circuit.split_temporary_before_insert(selection_id);
            editable_circuit.change_insertion_mode(selection_id, mode);
        }
    }

    return selection_id;
}

}  // namespace

auto InsertWireLogic::mouse_press(EditableCircuit& editable_circuit,
                                  std::optional<point_t> position) -> void {
    direction_.reset();
    first_position_ = position;

    temp_wire_ = remove_and_insert(editable_circuit, temp_wire_, first_position_,
                                   direction_, position, InsertionMode::collisions);
}

auto InsertWireLogic::mouse_move(EditableCircuit& editable_circuit,
                                 std::optional<point_t> position) -> void {
    if (position && first_position_) {
        if (position == first_position_) {
            direction_.reset();
        }

        if (position != first_position_ &&
            (!direction_ || is_orthogonal_line(*position, *first_position_))) {
            direction_ = is_horizontal(*position, *first_position_)
                             ? LineInsertionType::horizontal_first
                             : LineInsertionType::vertical_first;
        }
    }

    temp_wire_ = remove_and_insert(editable_circuit, temp_wire_, first_position_,
                                   direction_, position, InsertionMode::collisions);
}

auto InsertWireLogic::mouse_release(EditableCircuit& editable_circuit,
                                    std::optional<point_t> position) -> void {
    if (position && first_position_ && position == first_position_) {
        editable_circuit.toggle_inverter(*position);
        editable_circuit.toggle_wire_crosspoint(*position);
    }

    temp_wire_ =
        remove_and_insert(editable_circuit, temp_wire_, first_position_, direction_,
                          position, InsertionMode::insert_or_discard);

    save_destroy_selection(editable_circuit, temp_wire_);
    *this = {};
}

auto InsertWireLogic::finalize(EditableCircuit& editable_circuit) -> void {
    save_delete_all(editable_circuit, temp_wire_);
    save_destroy_selection(editable_circuit, temp_wire_);
    *this = {};
}

}  // namespace circuit_widget

}  // namespace logicsim
