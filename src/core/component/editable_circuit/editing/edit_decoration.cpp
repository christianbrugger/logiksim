#include "core/component/editable_circuit/editing/edit_decoration.h"

#include "core/component/editable_circuit/circuit_data.h"
#include "core/component/editable_circuit/editing/edit_decoration_detail.h"
#include "core/geometry/point.h"
#include "core/layout_info.h"
#include "core/selection.h"
#include "core/vocabulary/decoration_layout_data.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace logicsim {

namespace editable_circuit {

namespace editing {

//
// History
//

namespace {

auto _store_history_decoration_add_visible_selection(
    CircuitData& circuit, decoration_id_t decoration_id) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto decoration_key = circuit.index.key_index().get(decoration_id);

        if (circuit.visible_selection.initial_selection().is_selected(decoration_id)) {
            stack->push_decoration_add_visible_selection(decoration_key);
        }
    }
}

auto _store_history_decoration_remove_visible_selection(
    CircuitData& circuit, decoration_id_t decoration_id) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto decoration_key = circuit.index.key_index().get(decoration_id);

        if (!circuit.visible_selection.initial_selection().is_selected(decoration_id)) {
            stack->push_decoration_remove_visible_selection(decoration_key);
        }
    }
}

auto _store_history_create_decoration(CircuitData& circuit, decoration_id_t decoration_id,
                                      PlacedDecoration&& deleted_definition) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto decoration_key = circuit.index.key_index().get(decoration_id);

        if (circuit.visible_selection.initial_selection().is_selected(decoration_id)) {
            stack->push_decoration_add_visible_selection(decoration_key);
        }
        stack->push_decoration_create_temporary(decoration_key,
                                                std::move(deleted_definition));
    }
}

auto _store_history_move_temporary_decoration(CircuitData& circuit,
                                              decoration_id_t decoration_id,
                                              move_delta_t delta) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto decoration_key = circuit.index.key_index().get(decoration_id);
        stack->push_decoration_move_temporary(decoration_key, delta);
    }
}

auto _store_history_decoration_colliding_to_temporary(
    CircuitData& circuit, decoration_id_t decoration_id) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto decoration_key = circuit.index.key_index().get(decoration_id);
        stack->push_decoration_colliding_to_temporary(decoration_key);
    }
}

auto _store_history_decoration_temporary_to_colliding_expect_valid(
    CircuitData& circuit, decoration_id_t decoration_id) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto decoration_key = circuit.index.key_index().get(decoration_id);
        stack->push_decoration_temporary_to_colliding_expect_valid(decoration_key);
    }
}

auto _store_history_decoration_temporary_to_colliding_assume_colliding(
    CircuitData& circuit, decoration_id_t decoration_id) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto decoration_key = circuit.index.key_index().get(decoration_id);
        stack->push_decoration_temporary_to_colliding_assume_colliding(decoration_key);
    }
}

auto _store_history_decoration_insert_to_colliding_expect_valid(
    CircuitData& circuit, decoration_id_t decoration_id) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto decoration_key = circuit.index.key_index().get(decoration_id);
        stack->push_decoration_insert_to_colliding_expect_valid(decoration_key);
    }
}

auto _store_history_decoration_colliding_to_insert(
    CircuitData& circuit, decoration_id_t decoration_id) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto decoration_key = circuit.index.key_index().get(decoration_id);
        stack->push_decoration_colliding_to_insert(decoration_key);
    }
}

auto _store_history_delete_temporary_decoration(CircuitData& circuit,
                                                decoration_id_t decoration_id) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto decoration_key = circuit.index.key_index().get(decoration_id);
        stack->push_decoration_delete_temporary(decoration_key);
    }
}

auto _store_history_change_attribute_decoration(
    CircuitData& circuit, decoration_id_t decoration_id,
    attributes_text_element_t&& attrs) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto decoration_key = circuit.index.key_index().get(decoration_id);
        stack->push_decoration_change_attributes(decoration_key, std::move(attrs));
    }
}

}  // namespace

//
// Delete Decoration
//

namespace {

auto _notify_decoration_id_change(CircuitData& circuit,
                                  const decoration_id_t new_decoration_id,
                                  const decoration_id_t old_decoration_id) {
    circuit.submit(info_message::DecorationIdUpdated {
        .new_decoration_id = new_decoration_id,
        .old_decoration_id = old_decoration_id,
    });

    if (is_inserted(circuit.layout, new_decoration_id)) {
        const auto data = to_decoration_layout_data(circuit.layout, new_decoration_id);

        circuit.submit(info_message::InsertedDecorationIdUpdated {
            .new_decoration_id = new_decoration_id,
            .old_decoration_id = old_decoration_id,
            .data = data,
        });
    }
}

}  // namespace

auto delete_temporary_decoration(CircuitData& circuit,
                                 decoration_id_t& decoration_id) -> void {
    if (!decoration_id) [[unlikely]] {
        throw std::runtime_error("decoration id is invalid");
    }
    if (circuit.layout.decorations().display_state(decoration_id) !=
        display_state_t::temporary) [[unlikely]] {
        throw std::runtime_error("can only delete temporary objects");
    }

    auto [last_id, deleted_definition] =
        circuit.layout.decorations().swap_and_delete(decoration_id);

    _store_history_create_decoration(circuit, decoration_id,
                                     std::move(deleted_definition));

    circuit.submit(info_message::DecorationDeleted {decoration_id});

    if (decoration_id != last_id) {
        _notify_decoration_id_change(circuit, decoration_id, last_id);
    }

    // result
    decoration_id = null_decoration_id;
}

//
// Move Decoration
//

auto is_decoration_position_representable(const Layout& layout,
                                          const decoration_id_t decoration_id,
                                          move_delta_t delta) -> bool {
    if (!decoration_id) [[unlikely]] {
        throw std::runtime_error("element id is invalid");
    }

    const auto bounding_rect = layout.decorations().bounding_rect(decoration_id);

    return is_representable(bounding_rect.p0, delta.x, delta.y) &&
           is_representable(bounding_rect.p1, delta.x, delta.y);
}

auto are_decoration_positions_representable(const Layout& layout,
                                            const Selection& selection,
                                            move_delta_t delta) -> bool {
    if (delta == move_delta_t {0, 0}) {
        return true;
    }

    const auto decoration_valid = [&](decoration_id_t decoration_id) {
        return is_decoration_position_representable(layout, decoration_id, delta);
    };

    return std::ranges::all_of(selection.selected_decorations(), decoration_valid);
}

auto move_temporary_decoration_unchecked(CircuitData& circuit,
                                         const decoration_id_t decoration_id,
                                         move_delta_t delta) -> void {
    assert(std::as_const(circuit.layout).decorations().display_state(decoration_id) ==
           display_state_t::temporary);
    assert(is_decoration_position_representable(circuit.layout, decoration_id, delta));
    if (delta == move_delta_t {0, 0}) {
        return;
    }

    _store_history_move_temporary_decoration(circuit, decoration_id, -delta);

    const auto position = add_unchecked(
        circuit.layout.decorations().position(decoration_id), delta.x, delta.y);
    circuit.layout.decorations().set_position(decoration_id, position);
}

auto move_or_delete_temporary_decoration(CircuitData& circuit,
                                         decoration_id_t& decoration_id,
                                         move_delta_t delta) -> void {
    if (circuit.layout.decorations().display_state(decoration_id) !=
        display_state_t::temporary) [[unlikely]] {
        throw std::runtime_error("Only temporary items can be freely moved.");
    }
    if (delta == move_delta_t {0, 0}) {
        return;
    }

    if (!is_decoration_position_representable(circuit.layout, decoration_id, delta)) {
        delete_temporary_decoration(circuit, decoration_id);
        return;
    }

    move_temporary_decoration_unchecked(circuit, decoration_id, delta);
}

//
// Change Insertion Mode
//

namespace {

auto _decoration_change_temporary_to_colliding(CircuitData& circuit,
                                               const decoration_id_t decoration_id,
                                               InsertionHint hint) -> void {
    if (circuit.layout.decorations().display_state(decoration_id) !=
        display_state_t::temporary) [[unlikely]] {
        throw std::runtime_error("element is not in the right state.");
    }

    const auto is_colliding = is_decoration_colliding(circuit, decoration_id);
    if (is_colliding && hint == InsertionHint::expect_valid) [[unlikely]] {
        throw std::runtime_error("expect valid insert, but decoration is colliding");
    }

    _store_history_decoration_colliding_to_temporary(circuit, decoration_id);

    if (is_colliding || hint == InsertionHint::assume_colliding) {
        circuit.layout.decorations().set_display_state(decoration_id,
                                                       display_state_t::colliding);
        return;
    }

    circuit.layout.decorations().set_display_state(decoration_id, display_state_t::valid);
    circuit.submit(info_message::DecorationInserted {
        .decoration_id = decoration_id,
        .data = to_decoration_layout_data(circuit.layout, decoration_id),
    });
};

auto _decoration_change_colliding_to_temporary(CircuitData& circuit,
                                               decoration_id_t decoration_id) -> void;

auto _decoration_change_colliding_to_insert(CircuitData& circuit,
                                            decoration_id_t& decoration_id,
                                            InsertionHint hint) -> void {
    const auto display_state = circuit.layout.decorations().display_state(decoration_id);

    if (display_state != display_state_t::valid && hint == InsertionHint::expect_valid)
        [[unlikely]] {
        throw std::runtime_error("Expected decoration to be valid on insert");
    }

    if (display_state == display_state_t::valid) {
        _store_history_decoration_insert_to_colliding_expect_valid(circuit,
                                                                   decoration_id);
        circuit.layout.decorations().set_display_state(decoration_id,
                                                       display_state_t::normal);
        return;
    }

    if (display_state == display_state_t::colliding) [[likely]] {
        _decoration_change_colliding_to_temporary(circuit, decoration_id);
        delete_temporary_decoration(circuit, decoration_id);
        return;
    }

    throw std::runtime_error("element is not in the right state.");
};

auto _decoration_change_insert_to_colliding(CircuitData& circuit,
                                            const decoration_id_t decoration_id) -> void {
    if (circuit.layout.decorations().display_state(decoration_id) !=
        display_state_t::normal) [[unlikely]] {
        throw std::runtime_error("element is not in the right state.");
    }

    _store_history_decoration_colliding_to_insert(circuit, decoration_id);

    circuit.layout.decorations().set_display_state(decoration_id, display_state_t::valid);
};

auto _decoration_change_colliding_to_temporary(
    CircuitData& circuit, const decoration_id_t decoration_id) -> void {
    const auto display_state = circuit.layout.decorations().display_state(decoration_id);

    if (display_state == display_state_t::valid) {
        _store_history_decoration_temporary_to_colliding_expect_valid(circuit,
                                                                      decoration_id);

        circuit.submit(info_message::DecorationUninserted {
            .decoration_id = decoration_id,
            .data = to_decoration_layout_data(circuit.layout, decoration_id),
        });

        circuit.layout.decorations().set_display_state(decoration_id,
                                                       display_state_t::temporary);
        return;
    }

    if (display_state == display_state_t::colliding) {
        _store_history_decoration_temporary_to_colliding_assume_colliding(circuit,
                                                                          decoration_id);

        circuit.layout.decorations().set_display_state(decoration_id,
                                                       display_state_t::temporary);
        return;
    }

    throw std::runtime_error("element is not in the right state.");
};

}  // namespace

auto change_decoration_insertion_mode(CircuitData& circuit,
                                      decoration_id_t& decoration_id,
                                      InsertionMode new_mode,
                                      InsertionHint hint) -> void {
    if (!decoration_id) [[unlikely]] {
        throw std::runtime_error("element id is invalid");
    }
    if (!insertion_hint_valid(new_mode, hint)) [[unlikely]] {
        throw std::runtime_error("invalid insertion hint provided");
    }

    const auto old_mode =
        to_insertion_mode(circuit.layout.decorations().display_state(decoration_id));
    if (old_mode == new_mode) {
        return;
    }

    if (old_mode == InsertionMode::temporary) {
        _decoration_change_temporary_to_colliding(circuit, decoration_id, hint);
    }
    if (new_mode == InsertionMode::insert_or_discard) {
        _decoration_change_colliding_to_insert(circuit, decoration_id, hint);
    }
    if (old_mode == InsertionMode::insert_or_discard) {
        _decoration_change_insert_to_colliding(circuit, decoration_id);
    }
    if (new_mode == InsertionMode::temporary) {
        _decoration_change_colliding_to_temporary(circuit, decoration_id);
    }
}

//
// Add decoration
//

auto add_decoration(CircuitData& circuit, DecorationDefinition&& definition,
                    point_t position, InsertionMode insertion_mode,
                    decoration_key_t decoration_key) -> decoration_id_t {
    if (!is_representable(to_decoration_layout_data(definition, position))) {
        return null_decoration_id;
    }

    auto decoration_id = circuit.layout.decorations().add(std::move(definition), position,
                                                          display_state_t::temporary);
    circuit.submit(info_message::DecorationCreated {decoration_id});

    if (decoration_key) {
        circuit.index.set_key(decoration_id, decoration_key);
    }
    _store_history_delete_temporary_decoration(circuit, decoration_id);

    if (decoration_id) {
        change_decoration_insertion_mode(circuit, decoration_id, insertion_mode);
    }
    return decoration_id;
}

//
// Attributes
//

auto set_attributes_decoration(CircuitData& circuit, decoration_id_t decoration_id,
                               attributes_text_element_t&& attrs) -> void {
    auto old_attr =
        circuit.layout.decorations().set_attributes(decoration_id, std::move(attrs));

    _store_history_change_attribute_decoration(circuit, decoration_id,
                                               std::move(old_attr));
}

//
// Visible Selection
//

auto add_to_visible_selection(CircuitData& circuit_data,
                              decoration_id_t decoration_id) -> void {
    _store_history_decoration_remove_visible_selection(circuit_data, decoration_id);

    circuit_data.visible_selection.modify_initial_selection(
        [decoration_id](Selection& initial_selection) {
            initial_selection.add_decoration(decoration_id);
        });
}

auto remove_from_visible_selection(CircuitData& circuit_data,
                                   decoration_id_t decoration_id) -> void {
    _store_history_decoration_add_visible_selection(circuit_data, decoration_id);

    circuit_data.visible_selection.modify_initial_selection(
        [decoration_id](Selection& initial_selection) {
            initial_selection.remove_decoration(decoration_id);
        });
}

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim
