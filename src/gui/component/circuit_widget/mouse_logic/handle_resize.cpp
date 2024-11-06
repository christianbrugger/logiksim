#include "gui/component/circuit_widget/mouse_logic/handle_resize.h"

#include "core/algorithm/round.h"
#include "core/editable_circuit.h"
#include "core/geometry/display_state_map.h"
#include "core/vocabulary/point_fine.h"

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
      initial_element_ {get_single_placed_element(editable_circuit).value()} {
    Expects(editable_circuit.visible_selection().size() == 1);
    Expects(editable_circuit.visible_selection().selected_segments().empty());
    Expects(found_states_matches_insertion_mode(
        display_states(editable_circuit.visible_selection(), editable_circuit.layout()),
        InsertionMode::insert_or_discard));
}

auto HandleResizeLogic::mouse_press(EditableCircuit& editable_circuit [[maybe_unused]],
                                    point_fine_t position) -> void {
    first_position_ = position;
    last_delta_ = delta_movement_t {};
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

    editable_circuit.finish_undo_group();
}

namespace {

auto resize_element(EditableCircuit& editable_circuit, const PlacedElement& original,
                    size_handle_t size_handle, delta_movement_t new_delta) {
    // delete element
    editable_circuit.delete_all(editable_circuit.visible_selection());

    // add resized
    {
        auto new_element = get_resized_element(original, size_handle, new_delta);

        const auto guard = SelectionGuard {editable_circuit};
        add_placed_element(editable_circuit, std::move(new_element),
                           InsertionMode::temporary, guard.selection_id());
        editable_circuit.set_visible_selection(
            editable_circuit.selection(guard.selection_id()));
        // simplifies the history, to select first then change to colliding
        editable_circuit.change_insertion_mode(guard.selection_id(),
                                               InsertionMode::collisions);
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

    // TODO handle double out of range of int
    const auto new_delta = delta_movement_t {
        .horizontal = round_to<int>(double {position.x - first_position_->x}),
        .vertical = round_to<int>(double {position.y - first_position_->y}),
    };

    if (new_delta == *last_delta_) {
        return;
    }
    last_delta_ = new_delta;

    resize_element(editable_circuit, initial_element_, size_handle_, new_delta);
}

}  // namespace circuit_widget

}  // namespace logicsim
