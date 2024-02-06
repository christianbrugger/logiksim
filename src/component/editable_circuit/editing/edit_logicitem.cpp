#include "component/editable_circuit/editing/edit_logicitem.h"

#include "component/editable_circuit/circuit_data.h"
#include "component/editable_circuit/editing/edit_logicitem_detail.h"
#include "format/struct.h"
#include "geometry/orientation.h"
#include "geometry/point.h"
#include "layout_info.h"
#include "selection.h"
#include "vocabulary/logicitem_id.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace logicsim {

namespace editable_circuit {

namespace editing {

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

auto delete_temporary_logicitem(CircuitData& circuit, logicitem_id_t& logicitem_id,
                                logicitem_id_t* preserve_element) -> void {
    if (!logicitem_id) [[unlikely]] {
        throw std::runtime_error("logic item id is invalid");
    }

    if (circuit.layout.logic_items().display_state(logicitem_id) !=
        display_state_t::temporary) [[unlikely]] {
        throw std::runtime_error("can only delete temporary objects");
    }

    circuit.submit(info_message::LogicItemDeleted {logicitem_id});

    // delete in underlying
    auto last_id = circuit.layout.logic_items().swap_and_delete(logicitem_id);

    if (logicitem_id != last_id) {
        _notify_logicitem_id_change(circuit, logicitem_id, last_id);
    }

    if (preserve_element != nullptr) {
        if (*preserve_element == logicitem_id) {
            *preserve_element = null_logicitem_id;
        } else if (*preserve_element == last_id) {
            *preserve_element = logicitem_id;
        }
    }

    logicitem_id = null_logicitem_id;
}

//
// Move Logicitem
//

auto is_logicitem_position_representable(const Layout& layout,
                                         const logicitem_id_t logicitem_id, int dx,
                                         int dy) -> bool {
    if (!logicitem_id) [[unlikely]] {
        throw std::runtime_error("element id is invalid");
    }

    const auto position = layout.logic_items().position(logicitem_id);

    if (!is_representable(position, dx, dy)) {
        return false;
    }

    auto data = to_layout_calculation_data(layout, logicitem_id);
    data.position = add_unchecked(position, dx, dy);

    return is_representable(data);
}

auto are_logicitem_positions_representable(const Layout& layout,
                                           const Selection& selection, int delta_x,
                                           int delta_y) -> bool {
    const auto logic_item_valid = [&](logicitem_id_t logicitem_id) {
        return is_logicitem_position_representable(layout, logicitem_id, delta_x,
                                                   delta_y);
    };

    return std::ranges::all_of(selection.selected_logic_items(), logic_item_valid);
}

auto move_temporary_logicitem_unchecked(Layout& layout, const logicitem_id_t logicitem_id,
                                        int dx, int dy) -> void {
    assert(layout.logic_items().display_state(logicitem_id) ==
           display_state_t::temporary);
    assert(is_logicitem_position_representable(layout, logicitem_id, dx, dy));

    const auto position =
        add_unchecked(layout.logic_items().position(logicitem_id), dx, dy);
    layout.logic_items().set_position(logicitem_id, position);
}

auto move_or_delete_temporary_logicitem(CircuitData& circuit,
                                        logicitem_id_t& logicitem_id, int dx, int dy)
    -> void {
    if (circuit.layout.logic_items().display_state(logicitem_id) !=
        display_state_t::temporary) [[unlikely]] {
        throw std::runtime_error("Only temporary items can be freely moved.");
    }

    if (!is_logicitem_position_representable(circuit.layout, logicitem_id, dx, dy)) {
        delete_temporary_logicitem(circuit, logicitem_id);
        return;
    }

    move_temporary_logicitem_unchecked(circuit.layout, logicitem_id, dx, dy);
}

//
// Change Insertion Mode
//

namespace {

auto _element_change_temporary_to_colliding(CircuitData& circuit,
                                            const logicitem_id_t logicitem_id) -> void {
    if (circuit.layout.logic_items().display_state(logicitem_id) !=
        display_state_t::temporary) [[unlikely]] {
        throw std::runtime_error("element is not in the right state.");
    }

    if (is_logicitem_colliding(circuit, logicitem_id)) {
        circuit.layout.logic_items().set_display_state(logicitem_id,
                                                       display_state_t::colliding);
    } else {
        convert_wires_at_outputs_to_inputs(circuit, logicitem_id);
        circuit.layout.logic_items().set_display_state(logicitem_id,
                                                       display_state_t::valid);
        circuit.submit(info_message::LogicItemInserted {
            logicitem_id, to_layout_calculation_data(circuit.layout, logicitem_id)});
    }
};

auto _element_change_colliding_to_insert(CircuitData& circuit,
                                         logicitem_id_t& logicitem_id) -> void {
    const auto display_state = circuit.layout.logic_items().display_state(logicitem_id);

    if (display_state == display_state_t::valid) {
        circuit.layout.logic_items().set_display_state(logicitem_id,
                                                       display_state_t::normal);
        return;
    }

    if (display_state == display_state_t::colliding) [[likely]] {
        // we can only delete temporary elements
        circuit.layout.logic_items().set_display_state(logicitem_id,
                                                       display_state_t::temporary);
        delete_temporary_logicitem(circuit, logicitem_id);
        return;
    }

    throw std::runtime_error("element is not in the right state.");
};

auto _element_change_insert_to_colliding(Layout& layout,
                                         const logicitem_id_t logicitem_id) -> void {
    if (layout.logic_items().display_state(logicitem_id) != display_state_t::normal)
        [[unlikely]] {
        throw std::runtime_error("element is not in the right state.");
    }

    layout.logic_items().set_display_state(logicitem_id, display_state_t::valid);
};

auto _element_change_colliding_to_temporary(CircuitData& circuit,
                                            const logicitem_id_t logicitem_id) -> void {
    const auto display_state = circuit.layout.logic_items().display_state(logicitem_id);

    if (display_state == display_state_t::valid) {
        circuit.submit(info_message::LogicItemUninserted {
            logicitem_id, to_layout_calculation_data(circuit.layout, logicitem_id)});

        circuit.layout.logic_items().set_display_state(logicitem_id,
                                                       display_state_t::temporary);
        convert_wires_at_outputs_to_outputs(circuit, logicitem_id);
        return;
    }

    if (display_state == display_state_t::colliding) {
        circuit.layout.logic_items().set_display_state(logicitem_id,
                                                       display_state_t::temporary);
        return;
    }

    throw std::runtime_error("element is not in the right state.");
};

}  // namespace

auto change_logicitem_insertion_mode(CircuitData& circuit, logicitem_id_t& logicitem_id,
                                     InsertionMode new_mode) -> void {
    if (!logicitem_id) [[unlikely]] {
        throw std::runtime_error("element id is invalid");
    }

    const auto old_mode =
        to_insertion_mode(circuit.layout.logic_items().display_state(logicitem_id));
    if (old_mode == new_mode) {
        return;
    }

    if (old_mode == InsertionMode::temporary) {
        _element_change_temporary_to_colliding(circuit, logicitem_id);
    }
    if (new_mode == InsertionMode::insert_or_discard) {
        _element_change_colliding_to_insert(circuit, logicitem_id);
    }
    if (old_mode == InsertionMode::insert_or_discard) {
        _element_change_insert_to_colliding(circuit.layout, logicitem_id);
    }
    if (new_mode == InsertionMode::temporary) {
        _element_change_colliding_to_temporary(circuit, logicitem_id);
    }
}

//
// Add Logic Item
//

auto add_logicitem(CircuitData& circuit, const LogicItemDefinition& definition,
                   point_t position, InsertionMode insertion_mode) -> logicitem_id_t {
    // insert into underlying
    auto logicitem_id = circuit.layout.logic_items().add(definition, point_t {0, 0},
                                                         display_state_t::temporary);
    circuit.submit(info_message::LogicItemCreated {logicitem_id});

    // assume final position
    move_or_delete_temporary_logicitem(circuit, logicitem_id, int {position.x},
                                       int {position.y});
    if (logicitem_id) {
        change_logicitem_insertion_mode(circuit, logicitem_id, insertion_mode);
    }
    return logicitem_id;
}

//
// Toggle Inverter
//

auto toggle_inverter(CircuitData& circuit, point_t point) -> void {
    if (const auto entry = circuit.index.logicitem_input_index().find(point)) {
        const auto layout_data =
            to_layout_calculation_data(circuit.layout, entry->logicitem_id);
        const auto info = input_locations(layout_data).at(entry->connection_id.value);
        Expects(info.position == point);

        if (is_directed(info.orientation)) {
            const auto value = circuit.layout.logic_items().input_inverted(
                entry->logicitem_id, entry->connection_id);
            circuit.layout.logic_items().set_input_inverter(entry->logicitem_id,
                                                            entry->connection_id, !value);
        }
    }

    if (const auto entry = circuit.index.logicitem_output_index().find(point)) {
        const auto layout_data =
            to_layout_calculation_data(circuit.layout, entry->logicitem_id);
        const auto info = output_locations(layout_data).at(entry->connection_id.value);
        Expects(info.position == point);

        if (is_directed(info.orientation)) {
            const auto value = circuit.layout.logic_items().output_inverted(
                entry->logicitem_id, entry->connection_id);
            circuit.layout.logic_items().set_output_inverter(
                entry->logicitem_id, entry->connection_id, !value);
        }
    }
}

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim
