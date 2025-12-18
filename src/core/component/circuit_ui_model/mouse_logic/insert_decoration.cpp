#include "core/component/circuit_ui_model/mouse_logic/insert_decoration.h"

#include "core/component/circuit_ui_model/mouse_logic/mouse_logic_status.h"
#include "core/editable_circuit.h"
#include "core/vocabulary/insertion_mode.h"

#include <optional>

namespace logicsim {

namespace circuit_ui_model {

namespace {

auto remove_and_insert(EditableCircuit& editable_circuit, selection_id_t selection_id,
                       const DecorationDefinition& element_definition,
                       std::optional<point_t> position, InsertionMode mode)
    -> selection_id_t {
    save_delete_all(editable_circuit, selection_id);

    if (!editable_circuit.selection_exists(selection_id)) {
        selection_id = editable_circuit.create_selection();
    }

    if (position) {
        editable_circuit.add_decoration(DecorationDefinition {element_definition},
                                        position.value(), mode, selection_id);
    }

    return selection_id;
}

}  // namespace

InsertDecorationLogic::InsertDecorationLogic(DecorationDefinition element_definition)
    : element_definition_ {std::move(element_definition)} {}

auto InsertDecorationLogic::mouse_press(EditableCircuit& editable_circuit,
                                        std::optional<point_t> position) -> void {
    temp_element_ =
        remove_and_insert(editable_circuit, temp_element_, element_definition_, position,
                          InsertionMode::collisions);
}

auto InsertDecorationLogic::mouse_move(EditableCircuit& editable_circuit,
                                       std::optional<point_t> position) -> void {
    temp_element_ =
        remove_and_insert(editable_circuit, temp_element_, element_definition_, position,
                          InsertionMode::collisions);
}

auto InsertDecorationLogic::mouse_release(EditableCircuit& editable_circuit,
                                          std::optional<point_t> position)
    -> mouse_release_status_t {
    temp_element_ =
        remove_and_insert(editable_circuit, temp_element_, element_definition_, position,
                          InsertionMode::insert_or_discard);
    const auto inserted_decoration =
        get_single_decoration(editable_circuit, temp_element_);

    save_destroy_selection(editable_circuit, temp_element_);
    temp_element_ = null_selection_id;

    // TODO: open dialog
    static_cast<void>(inserted_decoration);
    // set_circuit_state(defaults::selection_state);
    // circuit_store_.editable_circuit().reopen_undo_group();
    // circuit_store_.editable_circuit().set_visible_selection(
    //     Selection {{}, std::array {result.inserted_decoration}});
    // circuit_store_.editable_circuit().finish_undo_group();

    return mouse_release_status_t {
        .finished = true,
        .mouse_logic_status =
            mouse_logic_status_t {
                .require_repaint = true,
                .dialogs_changed = true,
            },
    };
}

auto InsertDecorationLogic::finalize(EditableCircuit& editable_circuit) -> void {
    save_delete_all(editable_circuit, temp_element_);
    save_destroy_selection(editable_circuit, temp_element_);
    temp_element_ = null_selection_id;

    editable_circuit.finish_undo_group();
}

}  // namespace circuit_ui_model

}  // namespace logicsim
