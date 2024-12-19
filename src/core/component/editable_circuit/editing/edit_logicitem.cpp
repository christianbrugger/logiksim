#include "core/component/editable_circuit/editing/edit_logicitem.h"

#include "core/component/editable_circuit/circuit_data.h"
#include "core/component/editable_circuit/editing/edit_logicitem_detail.h"
#include "core/format/struct.h"
#include "core/geometry/orientation.h"
#include "core/geometry/point.h"
#include "core/layout_info.h"
#include "core/selection.h"
#include "core/vocabulary/logicitem_id.h"

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

auto _store_history_logicitem_add_visible_selection(CircuitData& circuit,
                                                    logicitem_id_t logicitem_id) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto logicitem_key = circuit.index.key_index().get(logicitem_id);

        if (circuit.visible_selection.initial_selection().is_selected(logicitem_id)) {
            stack->push_logicitem_add_visible_selection(logicitem_key);
        }
    }
}

auto _store_history_logicitem_remove_visible_selection(
    CircuitData& circuit, logicitem_id_t logicitem_id) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto logicitem_key = circuit.index.key_index().get(logicitem_id);

        if (!circuit.visible_selection.initial_selection().is_selected(logicitem_id)) {
            stack->push_logicitem_remove_visible_selection(logicitem_key);
        }
    }
}

auto _store_history_create_logicitem(CircuitData& circuit, logicitem_id_t logicitem_id,
                                     PlacedLogicItem&& deleted_definition) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto logicitem_key = circuit.index.key_index().get(logicitem_id);

        if (circuit.visible_selection.initial_selection().is_selected(logicitem_id)) {
            stack->push_logicitem_add_visible_selection(logicitem_key);
        }
        stack->push_logicitem_create_temporary(logicitem_key,
                                               std::move(deleted_definition));
    }
}

auto _store_history_move_temporary_logicitem(CircuitData& circuit,
                                             logicitem_id_t logicitem_id,
                                             move_delta_t delta) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto logicitem_key = circuit.index.key_index().get(logicitem_id);
        stack->push_logicitem_move_temporary(logicitem_key, delta);
    }
}

auto _store_history_logicitem_colliding_to_temporary(
    CircuitData& circuit, logicitem_id_t logicitem_id) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto logicitem_key = circuit.index.key_index().get(logicitem_id);
        stack->push_logicitem_colliding_to_temporary(logicitem_key);
    }
}

auto _store_history_logicitem_temporary_to_colliding_expect_valid(
    CircuitData& circuit, logicitem_id_t logicitem_id) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto logicitem_key = circuit.index.key_index().get(logicitem_id);
        stack->push_logicitem_temporary_to_colliding_expect_valid(logicitem_key);
    }
}

auto _store_history_logicitem_temporary_to_colliding_assume_colliding(
    CircuitData& circuit, logicitem_id_t logicitem_id) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto logicitem_key = circuit.index.key_index().get(logicitem_id);
        stack->push_logicitem_temporary_to_colliding_assume_colliding(logicitem_key);
    }
}

auto _store_history_logicitem_insert_to_colliding_expect_valid(
    CircuitData& circuit, logicitem_id_t logicitem_id) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto logicitem_key = circuit.index.key_index().get(logicitem_id);
        stack->push_logicitem_insert_to_colliding_expect_valid(logicitem_key);
    }
}

auto _store_history_logicitem_colliding_to_insert(CircuitData& circuit,
                                                  logicitem_id_t logicitem_id) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto logicitem_key = circuit.index.key_index().get(logicitem_id);
        stack->push_logicitem_colliding_to_insert(logicitem_key);
    }
}

auto _store_history_delete_temporary_logicitem(CircuitData& circuit,
                                               logicitem_id_t logicitem_id) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto logicitem_key = circuit.index.key_index().get(logicitem_id);
        stack->push_logicitem_delete_temporary(logicitem_key);
    }
}

auto _store_history_change_attribute_logicitem(
    CircuitData& circuit, logicitem_id_t logicitem_id,
    attributes_clock_generator_t&& attrs) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto logicitem_key = circuit.index.key_index().get(logicitem_id);
        stack->push_logicitem_change_attributes(logicitem_key, std::move(attrs));
    }
}

auto _store_history_logicitem_loggle_inverter(CircuitData& circuit,
                                              logicitem_id_t logicitem_id) -> void {
    // inverter toggles are rare event, nut performed in bulk
    // so other history events are re-used to map it
    if (const auto stack = circuit.history.get_stack()) {
        const auto logicitem_key = circuit.index.key_index().get(logicitem_id);
        const auto state = circuit.layout.logicitems().display_state(logicitem_id);

        // create old one
        if (state == display_state_t::normal) {
            stack->push_logicitem_colliding_to_insert(logicitem_key);
        }
        if (state != display_state_t::temporary) {
            stack->push_logicitem_temporary_to_colliding_expect_valid(logicitem_key);
        }
        stack->push_logicitem_create_temporary(
            logicitem_key, to_placed_logicitem(circuit.layout, logicitem_id));

        // delete new one
        stack->push_logicitem_delete_temporary(logicitem_key);
        if (state != display_state_t::temporary) {
            stack->push_logicitem_colliding_to_temporary(logicitem_key);
        }
        if (state == display_state_t::normal) {
            stack->push_logicitem_insert_to_colliding_expect_valid(logicitem_key);
        }
    }
}

}  // namespace

//
// Delete Logicitem
//

namespace {

auto _notify_logicitem_id_change(CircuitData& circuit,
                                 const logicitem_id_t new_logicitem_id,
                                 const logicitem_id_t old_logicitem_id) {
    circuit.submit(info_message::LogicItemIdUpdated {
        .new_logicitem_id = new_logicitem_id,
        .old_logicitem_id = old_logicitem_id,
    });

    if (is_inserted(circuit.layout, new_logicitem_id)) {
        const auto data = to_layout_calculation_data(circuit.layout, new_logicitem_id);

        circuit.submit(info_message::InsertedLogicItemIdUpdated {
            .new_logicitem_id = new_logicitem_id,
            .old_logicitem_id = old_logicitem_id,
            .data = data,
        });
    }
}

}  // namespace

auto delete_temporary_logicitem(CircuitData& circuit,
                                logicitem_id_t& logicitem_id) -> void {
    if (!logicitem_id) [[unlikely]] {
        throw std::runtime_error("logicitem id is invalid");
    }

    if (circuit.layout.logicitems().display_state(logicitem_id) !=
        display_state_t::temporary) [[unlikely]] {
        throw std::runtime_error("can only delete temporary objects");
    }

    auto [last_id, deleted_definition] =
        circuit.layout.logicitems().swap_and_delete(logicitem_id);

    _store_history_create_logicitem(circuit, logicitem_id, std::move(deleted_definition));

    circuit.submit(info_message::LogicItemDeleted {logicitem_id});

    if (logicitem_id != last_id) {
        _notify_logicitem_id_change(circuit, logicitem_id, last_id);
    }

    logicitem_id = null_logicitem_id;
}

//
// Move Logicitem
//

auto is_logicitem_position_representable(const Layout& layout,
                                         const logicitem_id_t logicitem_id,
                                         move_delta_t delta) -> bool {
    if (!logicitem_id) [[unlikely]] {
        throw std::runtime_error("element id is invalid");
    }

    const auto position = layout.logicitems().position(logicitem_id);

    if (!is_representable(position, delta.x, delta.y)) {
        return false;
    }

    auto data = to_layout_calculation_data(layout, logicitem_id);
    data.position = add_unchecked(position, delta.x, delta.y);

    return is_representable(data);
}

auto are_logicitem_positions_representable(const Layout& layout,
                                           const Selection& selection,
                                           move_delta_t delta) -> bool {
    const auto logicitem_valid = [&](logicitem_id_t logicitem_id) {
        return is_logicitem_position_representable(layout, logicitem_id, delta);
    };

    return std::ranges::all_of(selection.selected_logicitems(), logicitem_valid);
}

auto move_temporary_logicitem_unchecked(CircuitData& circuit,
                                        const logicitem_id_t logicitem_id,
                                        move_delta_t delta) -> void {
    assert(std::as_const(circuit.layout).logicitems().display_state(logicitem_id) ==
           display_state_t::temporary);
    assert(is_logicitem_position_representable(circuit.layout, logicitem_id, delta));

    _store_history_move_temporary_logicitem(circuit, logicitem_id, -delta);

    const auto position = add_unchecked(
        circuit.layout.logicitems().position(logicitem_id), delta.x, delta.y);
    circuit.layout.logicitems().set_position(logicitem_id, position);
}

auto move_or_delete_temporary_logicitem(CircuitData& circuit,
                                        logicitem_id_t& logicitem_id,
                                        move_delta_t delta) -> void {
    if (circuit.layout.logicitems().display_state(logicitem_id) !=
        display_state_t::temporary) [[unlikely]] {
        throw std::runtime_error("Only temporary items can be freely moved.");
    }
    if (delta == move_delta_t {0, 0}) {
        return;
    }

    if (!is_logicitem_position_representable(circuit.layout, logicitem_id, delta)) {
        delete_temporary_logicitem(circuit, logicitem_id);
        return;
    }

    move_temporary_logicitem_unchecked(circuit, logicitem_id, delta);
}

//
// Change Insertion Mode
//

namespace {

auto _element_change_temporary_to_colliding(CircuitData& circuit,
                                            const logicitem_id_t logicitem_id,
                                            InsertionHint hint) -> void {
    if (circuit.layout.logicitems().display_state(logicitem_id) !=
        display_state_t::temporary) [[unlikely]] {
        throw std::runtime_error("element is not in the right state.");
    }

    const auto is_colliding = is_logicitem_colliding(circuit, logicitem_id);
    if (is_colliding && hint == InsertionHint::expect_valid) [[unlikely]] {
        throw std::runtime_error("expect valid insert, but logicitem is colliding");
    }

    _store_history_logicitem_colliding_to_temporary(circuit, logicitem_id);

    if (is_colliding || hint == InsertionHint::assume_colliding) {
        circuit.layout.logicitems().set_display_state(logicitem_id,
                                                      display_state_t::colliding);
        return;
    }

    convert_wires_at_outputs_to_inputs(circuit, logicitem_id);
    circuit.layout.logicitems().set_display_state(logicitem_id, display_state_t::valid);
    circuit.submit(info_message::LogicItemInserted {
        logicitem_id, to_layout_calculation_data(circuit.layout, logicitem_id)});
};

auto _element_change_colliding_to_temporary(CircuitData& circuit,
                                            logicitem_id_t logicitem_id) -> void;

auto _element_change_colliding_to_insert(CircuitData& circuit,
                                         logicitem_id_t& logicitem_id,
                                         InsertionHint hint) -> void {
    const auto display_state = circuit.layout.logicitems().display_state(logicitem_id);

    if (display_state != display_state_t::valid && hint == InsertionHint::expect_valid)
        [[unlikely]] {
        throw std::runtime_error("Expected logicitem to be valid on insert");
    }

    if (display_state == display_state_t::valid) {
        _store_history_logicitem_insert_to_colliding_expect_valid(circuit, logicitem_id);
        circuit.layout.logicitems().set_display_state(logicitem_id,
                                                      display_state_t::normal);
        return;
    }

    if (display_state == display_state_t::colliding) [[likely]] {
        _element_change_colliding_to_temporary(circuit, logicitem_id);
        delete_temporary_logicitem(circuit, logicitem_id);
        return;
    }

    throw std::runtime_error("element is not in the right state.");
};

auto _element_change_insert_to_colliding(CircuitData& circuit,
                                         const logicitem_id_t logicitem_id) -> void {
    if (circuit.layout.logicitems().display_state(logicitem_id) !=
        display_state_t::normal) [[unlikely]] {
        throw std::runtime_error("element is not in the right state.");
    }

    _store_history_logicitem_colliding_to_insert(circuit, logicitem_id);

    circuit.layout.logicitems().set_display_state(logicitem_id, display_state_t::valid);
};

auto _element_change_colliding_to_temporary(CircuitData& circuit,
                                            const logicitem_id_t logicitem_id) -> void {
    const auto display_state = circuit.layout.logicitems().display_state(logicitem_id);

    if (display_state == display_state_t::valid) {
        _store_history_logicitem_temporary_to_colliding_expect_valid(circuit,
                                                                     logicitem_id);

        circuit.submit(info_message::LogicItemUninserted {
            .logicitem_id = logicitem_id,
            .data = to_layout_calculation_data(circuit.layout, logicitem_id),
        });

        circuit.layout.logicitems().set_display_state(logicitem_id,
                                                      display_state_t::temporary);
        convert_wires_at_outputs_to_outputs(circuit, logicitem_id);
        return;
    }

    if (display_state == display_state_t::colliding) {
        _store_history_logicitem_temporary_to_colliding_assume_colliding(circuit,
                                                                         logicitem_id);

        circuit.layout.logicitems().set_display_state(logicitem_id,
                                                      display_state_t::temporary);
        return;
    }

    throw std::runtime_error("element is not in the right state.");
};

}  // namespace

auto change_logicitem_insertion_mode(CircuitData& circuit, logicitem_id_t& logicitem_id,
                                     InsertionMode new_mode, InsertionHint hint) -> void {
    if (!logicitem_id) [[unlikely]] {
        throw std::runtime_error("element id is invalid");
    }
    if (!insertion_hint_valid(new_mode, hint)) [[unlikely]] {
        throw std::runtime_error("invalid insertion hint provided");
    }

    const auto old_mode =
        to_insertion_mode(circuit.layout.logicitems().display_state(logicitem_id));
    if (old_mode == new_mode) {
        return;
    }

    if (old_mode == InsertionMode::temporary) {
        _element_change_temporary_to_colliding(circuit, logicitem_id, hint);
    }
    if (new_mode == InsertionMode::insert_or_discard) {
        _element_change_colliding_to_insert(circuit, logicitem_id, hint);
    }
    if (old_mode == InsertionMode::insert_or_discard) {
        _element_change_insert_to_colliding(circuit, logicitem_id);
    }
    if (new_mode == InsertionMode::temporary) {
        _element_change_colliding_to_temporary(circuit, logicitem_id);
    }
}

//
// Add Logic Item
//

auto add_logicitem(CircuitData& circuit, LogicItemDefinition&& definition,
                   point_t position, InsertionMode insertion_mode,
                   logicitem_key_t logicitem_key) -> logicitem_id_t {
    if (!is_representable(to_layout_calculation_data(definition, position))) {
        return null_logicitem_id;
    }

    auto logicitem_id = circuit.layout.logicitems().add(std::move(definition), position,
                                                        display_state_t::temporary);
    circuit.submit(info_message::LogicItemCreated {logicitem_id});

    if (logicitem_key) {
        circuit.index.set_key(logicitem_id, logicitem_key);
    }
    _store_history_delete_temporary_logicitem(circuit, logicitem_id);

    if (logicitem_id) {
        change_logicitem_insertion_mode(circuit, logicitem_id, insertion_mode);
    }
    return logicitem_id;
}

//
// Toggle Inverter
//

namespace {

auto _get_toggleable_input(const CircuitData& circuit,
                           point_t point) -> std::optional<logicitem_connection_t> {
    if (const auto entry = circuit.index.logicitem_input_index().find(point)) {
        const auto layout_data =
            to_layout_calculation_data(circuit.layout, entry->logicitem_id);

        const auto info = input_locations(layout_data).at(entry->connection_id.value);
        Expects(info.position == point);

        if (is_directed(info.orientation)) {
            return entry;
        }
    }

    return std::nullopt;
}

auto _get_toggleable_output(const CircuitData& circuit,
                            point_t point) -> std::optional<logicitem_connection_t> {
    if (const auto entry = circuit.index.logicitem_output_index().find(point)) {
        const auto layout_data =
            to_layout_calculation_data(circuit.layout, entry->logicitem_id);

        const auto info = output_locations(layout_data).at(entry->connection_id.value);
        Expects(info.position == point);

        if (is_directed(info.orientation)) {
            return entry;
        }
    }

    return std::nullopt;
}

}  // namespace

auto toggle_inverter(CircuitData& circuit, point_t point) -> void {
    // input
    if (const auto entry = _get_toggleable_input(circuit, point)) {
        _store_history_logicitem_loggle_inverter(circuit, entry->logicitem_id);

        const auto value = circuit.layout.logicitems().input_inverted(
            entry->logicitem_id, entry->connection_id);
        circuit.layout.logicitems().set_input_inverter(entry->logicitem_id,
                                                       entry->connection_id, !value);
        return;
    }

    // output
    if (const auto entry = _get_toggleable_output(circuit, point)) {
        _store_history_logicitem_loggle_inverter(circuit, entry->logicitem_id);

        const auto value = circuit.layout.logicitems().output_inverted(
            entry->logicitem_id, entry->connection_id);
        circuit.layout.logicitems().set_output_inverter(entry->logicitem_id,
                                                        entry->connection_id, !value);
        return;
    }
}

//
// Attributes
//

auto set_attributes_logicitem(CircuitData& circuit, logicitem_id_t logicitem_id,
                              attributes_clock_generator_t&& attrs) -> void {
    auto old_attr =
        circuit.layout.logicitems().set_attributes(logicitem_id, std::move(attrs));

    _store_history_change_attribute_logicitem(circuit, logicitem_id, std::move(old_attr));
}

//
// Visible Selection
//

auto add_to_visible_selection(CircuitData& circuit_data,
                              logicitem_id_t logicitem_id) -> void {
    _store_history_logicitem_remove_visible_selection(circuit_data, logicitem_id);

    circuit_data.visible_selection.modify_initial_selection(
        [logicitem_id](Selection& initial_selection) {
            initial_selection.add_logicitem(logicitem_id);
        });
}

auto remove_from_visible_selection(CircuitData& circuit_data,
                                   logicitem_id_t logicitem_id) -> void {
    _store_history_logicitem_add_visible_selection(circuit_data, logicitem_id);

    circuit_data.visible_selection.modify_initial_selection(
        [logicitem_id](Selection& initial_selection) {
            initial_selection.remove_logicitem(logicitem_id);
        });
}

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim
