#include "component/editable_circuit/editing/edit_logicitem.h"

#include "component/editable_circuit/circuit_data.h"
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

namespace {

//
// Delete Logicitem
//

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
// Wire conversion
//

namespace {

struct wire_connection_t {
    point_t position;
    segment_t segment;

    [[nodiscard]] auto format() const -> std::string;
};

auto wire_connection_t::format() const -> std::string {
    return fmt::format("({}, {})", position, segment);
}

using policy = folly::small_vector_policy::policy_size_type<uint32_t>;
using wire_connections_t = folly::small_vector<wire_connection_t, 3, policy>;

static_assert(sizeof(wire_connection_t) == 12);
static_assert(sizeof(wire_connections_t) == 40);

auto has_duplicate_wire_ids(wire_connections_t connections) -> bool {
    auto to_wire_id = [](wire_connection_t input) { return input.segment.wire_id; };

    std::ranges::sort(connections, std::ranges::less {}, to_wire_id);

    return std::ranges::adjacent_find(connections, std::ranges::equal_to {},
                                      to_wire_id) != connections.end();
}

auto is_convertible_to_input(const Layout& layout, wire_id_t wire_id) -> bool {
    return !layout.wires().segment_tree(wire_id).has_input();
}

auto all_convertible_to_input(const Layout& layout, wire_connections_t connections)
    -> bool {
    return std::ranges::all_of(connections, [&](wire_connection_t input) {
        return is_convertible_to_input(layout, input.segment.wire_id);
    });
}

struct convertible_inputs_result_t {
    wire_connections_t convertible_inputs {};
    bool any_collisions {false};

    [[nodiscard]] auto format() const -> std::string;
};

auto convertible_inputs_result_t::format() const -> std::string {
    return fmt::format("<any_collisions = {}, convertible_inputs = {}>", any_collisions,
                       convertible_inputs);
}

auto _find_convertible_wire_input_candidates(const LayoutIndex& index,
                                             const layout_calculation_data_t& data)
    -> convertible_inputs_result_t {
    auto result = convertible_inputs_result_t {};

    for (const auto& info : output_locations(data)) {
        if (const auto entry = index.wire_output_index().find(info.position)) {
            // not compatible
            if (!orientations_compatible(info.orientation, entry->orientation)) {
                return {.any_collisions = true};
            }

            result.convertible_inputs.push_back({info.position, entry->segment});
        }
    }

    return result;
}

auto find_convertible_wire_inputs(const CircuitData& circuit,
                                  const layout_calculation_data_t& data)
    -> convertible_inputs_result_t {
    auto candidates = _find_convertible_wire_input_candidates(circuit.index, data);

    if (candidates.any_collisions ||
        has_duplicate_wire_ids(candidates.convertible_inputs) ||
        !all_convertible_to_input(circuit.layout, candidates.convertible_inputs)) {
        return {.any_collisions = true};
    }

    return candidates;
}

auto assert_equal_type(SegmentPointType type, SegmentPointType expected) -> void {
    if (type != expected) [[unlikely]] {
        throw std::runtime_error("type is not of expected type");
    }
}

auto convert_from_to(CircuitData& circuit, wire_connection_t output,
                     SegmentPointType from_type, SegmentPointType to_type) {
    if (!is_inserted(output.segment.wire_id)) [[unlikely]] {
        throw std::runtime_error("can only convert inserted wires");
    }

    auto& m_tree = circuit.layout.wires().modifiable_segment_tree(output.segment.wire_id);
    const auto old_info = m_tree.info(output.segment.segment_index);
    auto new_info = old_info;

    if (new_info.line.p0 == output.position) {
        assert_equal_type(new_info.p0_type, from_type);
        new_info.p0_type = to_type;
    }

    else if (new_info.line.p1 == output.position) {
        assert_equal_type(new_info.p1_type, from_type);
        new_info.p1_type = to_type;
    }

    else [[unlikely]] {
        throw std::runtime_error("connector position is not part of segment line");
    }

    m_tree.update_segment(output.segment.segment_index, new_info);

    circuit.submit(info_message::InsertedEndPointsUpdated {
        .segment = output.segment,
        .new_segment_info = new_info,
        .old_segment_info = old_info,
    });
}

auto convert_to_input(CircuitData& circuit, wire_connection_t output) {
    convert_from_to(circuit, output, SegmentPointType::output, SegmentPointType::input);
}

auto convert_to_output(CircuitData& circuit, wire_connection_t output) {
    convert_from_to(circuit, output, SegmentPointType::input, SegmentPointType::output);
}

auto convert_to_inputs(CircuitData& circuit, wire_connections_t outputs) {
    for (auto output : outputs) {
        convert_to_input(circuit, output);
    }
}

}  // namespace

//
// Change Insertion Mode
//

namespace {

auto _any_logicitem_inputs_colliding(const LayoutIndex& cache,
                                     const layout_calculation_data_t& data) -> bool {
    const auto compatible = [&](simple_input_info_t info) -> bool {
        if (const auto entry = cache.wire_output_index().find(info.position)) {
            return orientations_compatible(info.orientation, entry->orientation);
        }
        return true;
    };

    return !std::ranges::all_of(input_locations(data), compatible);
}

auto _any_logicitem_outputs_colliding(CircuitData& circuit,
                                      const layout_calculation_data_t& data) -> bool {
    return find_convertible_wire_inputs(circuit, data).any_collisions;
}

auto is_logicitem_colliding(CircuitData& circuit, const logicitem_id_t logicitem_id) {
    const auto data = to_layout_calculation_data(circuit.layout, logicitem_id);

    return circuit.index.collision_index().is_colliding(data) ||
           _any_logicitem_inputs_colliding(circuit.index, data) ||
           _any_logicitem_outputs_colliding(circuit, data);
}

auto insert_logicitem_wire_conversion(CircuitData& circuit,
                                      const logicitem_id_t logicitem_id) {
    const auto data = to_layout_calculation_data(circuit.layout, logicitem_id);

    auto result = find_convertible_wire_inputs(circuit, data);

    // we assume there will be no collision at this point
    if (result.any_collisions) [[unlikely]] {
        throw std::runtime_error("inserted logic item is colliding");
    }

    convert_to_inputs(circuit, result.convertible_inputs);
}

auto uninsert_logicitem_wire_conversion(CircuitData& circuit,
                                        const logicitem_id_t logicitem_id) -> void {
    const auto data = to_layout_calculation_data(circuit.layout, logicitem_id);

    for (auto info : output_locations(data)) {
        if (const auto entry = circuit.index.wire_input_index().find(info.position)) {
            const auto connection = wire_connection_t {info.position, entry->segment};
            convert_to_output(circuit, connection);
        }
    }
}

auto notify_logicitem_inserted(CircuitData& circuit, const logicitem_id_t logicitem_id) {
    const auto data = to_layout_calculation_data(circuit.layout, logicitem_id);
    circuit.submit(info_message::LogicItemInserted {logicitem_id, data});
}

auto notify_logicitem_uninserted(CircuitData& circuit,
                                 const logicitem_id_t logicitem_id) {
    const auto data = to_layout_calculation_data(circuit.layout, logicitem_id);
    circuit.submit(info_message::LogicItemUninserted {logicitem_id, data});
}

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
        insert_logicitem_wire_conversion(circuit, logicitem_id);
        circuit.layout.logic_items().set_display_state(logicitem_id,
                                                       display_state_t::valid);
        notify_logicitem_inserted(circuit, logicitem_id);
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
        notify_logicitem_uninserted(circuit, logicitem_id);
        circuit.layout.logic_items().set_display_state(logicitem_id,
                                                       display_state_t::temporary);
        uninsert_logicitem_wire_conversion(circuit, logicitem_id);
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
