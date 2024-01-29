#include "component/circuit_widget/mouse_logic/handle_resize.h"

#include "algorithm/round.h"
#include "editable_circuit.h"
#include "geometry/display_state_map.h"
#include "vocabulary/point_fine.h"

namespace logicsim {

namespace circuit_widget {

namespace {
auto visible_selection_colliding(const EditableCircuit& editable_circuit) {
    return anything_colliding(editable_circuit.visible_selection(),
                              editable_circuit.layout());
}
}  // namespace

HandleResizeLogic::HandleResizeLogic(const EditableCircuit& editable_circuit,
                                     size_handle_t size_handle)
    : size_handle_ {size_handle},
      initial_logic_item_ {get_single_placed_element(editable_circuit)} {
    Expects(editable_circuit.visible_selection().selected_logic_items().size() == 1);
    Expects(editable_circuit.visible_selection().selected_segments().size() == 0);
    Expects(found_states_matches_insertion_mode(
        display_states(editable_circuit.visible_selection(), editable_circuit.layout()),
        InsertionMode::insert_or_discard));
}

auto HandleResizeLogic::mouse_press(EditableCircuit& editable_circuit,
                                    point_fine_t position) -> void {
    first_position_ = position;
    last_delta_ = 0;
}

auto HandleResizeLogic::mouse_move(EditableCircuit& editable_circuit,
                                   point_fine_t position) -> void {
    move_handle_to(editable_circuit, position);
}

auto HandleResizeLogic::mouse_release(EditableCircuit& editable_circuit,
                                      point_fine_t position) -> void {
    move_handle_to(editable_circuit, position);

    // mark as permanent
    if (!visible_selection_colliding(editable_circuit)) {
        first_position_.reset();
        last_delta_.reset();
    }
}

auto HandleResizeLogic::finalize(EditableCircuit& editable_circuit) -> void {
    if (first_position_) {
        move_handle_to(editable_circuit, first_position_.value());
    }

    Expects(found_states_matches_insertion_mode(
        display_states(editable_circuit.visible_selection(), editable_circuit.layout()),
        InsertionMode::insert_or_discard));
}

namespace {

auto resize_logic_item(EditableCircuit& editable_circuit, const PlacedElement original,
                       size_handle_t size_handle, int new_delta) {
    // delete element
    editable_circuit.delete_all(editable_circuit.visible_selection());

    // add resized
    {
        const auto logic_item = get_resized_element(original, size_handle, new_delta);
        const auto sel_handle = ScopedSelection {editable_circuit};
        editable_circuit.add_logic_item(logic_item.definition, logic_item.position,
                                        InsertionMode::collisions,
                                        sel_handle.selection_id());
        editable_circuit.set_visible_selection(
            editable_circuit.selection(sel_handle.selection_id()));
    }

    // check collisions
    if (!visible_selection_colliding(editable_circuit)) {
        editable_circuit.change_insertion_mode(editable_circuit.visible_selection(),
                                               InsertionMode::insert_or_discard);
    }
}

}  // namespace

auto HandleResizeLogic::move_handle_to(EditableCircuit& editable_circuit,
                                       point_fine_t position) -> void {
    if (!first_position_ || !last_delta_) {
        return;
    }

    const auto new_delta = round_to<int>(double {position.y - first_position_->y});
    if (new_delta == *last_delta_) {
        return;
    }
    last_delta_ = new_delta;

    resize_logic_item(editable_circuit, initial_logic_item_, size_handle_, new_delta);
}

}  // namespace circuit_widget

}  // namespace logicsim
