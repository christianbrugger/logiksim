#include "core/component/editable_circuit/editing/edit_logicitem_detail.h"

#include "core/component/editable_circuit/circuit_data.h"
#include "core/component/editable_circuit/layout_index.h"
#include "core/geometry/orientation.h"
#include "core/layout.h"
#include "core/layout_info.h"

#include <algorithm>
#include <functional>

namespace logicsim {

namespace editable_circuit {

namespace editing {

//
// Wire Connection
//

auto wire_connection_t::format() const -> std::string {
    return fmt::format("({}, {})", position, segment);
}

auto has_duplicate_wire_ids(wire_connections_t connections) -> bool {
    auto to_wire_id = [](wire_connection_t input) { return input.segment.wire_id; };

    std::ranges::sort(connections, std::ranges::less {}, to_wire_id);

    return std::ranges::adjacent_find(connections, std::ranges::equal_to {},
                                      to_wire_id) != connections.end();
}

//
// Check Convertible Inputs
//

auto is_convertible_to_input(const Layout& layout, wire_id_t wire_id) -> bool {
    return !layout.wires().segment_tree(wire_id).has_input();
}

auto all_convertible_to_input(const Layout& layout, const wire_connections_t& connections)
    -> bool {
    return std::ranges::all_of(connections, [&](wire_connection_t input) {
        return is_convertible_to_input(layout, input.segment.wire_id);
    });
}

auto convertible_inputs_result_t::format() const -> std::string {
    return fmt::format("<any_collisions = {}, convertible_inputs = {}>", any_collisions,
                       convertible_inputs);
}

namespace {

auto _find_convertible_wire_input_candidates(const LayoutIndex& index,
                                             const layout_calculation_data_t& data)
    -> convertible_inputs_result_t {
    auto result = convertible_inputs_result_t {.any_collisions = false};

    for (const auto& info : output_locations(data)) {
        if (const auto entry = index.wire_output_index().find(info.position)) {
            if (!orientations_compatible(info.orientation, entry->orientation)) {
                return {.any_collisions = true};
            }

            result.convertible_inputs.push_back({info.position, entry->segment});
        }
    }

    return result;
}

}  // namespace

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

//
// Convert Inputs / Outputs
//

namespace {

auto _ensure_equal_type(SegmentPointType type, SegmentPointType expected) -> void {
    if (type != expected) [[unlikely]] {
        throw std::runtime_error("type is not of expected type");
    }
}

}  // namespace

auto convert_from_to(CircuitData& circuit, wire_connection_t output,
                     SegmentPointType from_type, SegmentPointType to_type) -> void {
    if (!is_inserted(output.segment.wire_id)) [[unlikely]] {
        throw std::runtime_error("can only convert inserted wires");
    }

    auto& m_tree = circuit.layout.wires().modifiable_segment_tree(output.segment.wire_id);
    const auto old_info = m_tree.info(output.segment.segment_index);
    auto new_info = old_info;

    if (new_info.line.p0 == output.position) {
        _ensure_equal_type(new_info.p0_type, from_type);
        new_info.p0_type = to_type;
    }

    else if (new_info.line.p1 == output.position) {
        _ensure_equal_type(new_info.p1_type, from_type);
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

auto convert_to_input(CircuitData& circuit, wire_connection_t output) -> void {
    convert_from_to(circuit, output, SegmentPointType::output, SegmentPointType::input);
}

auto convert_to_output(CircuitData& circuit, wire_connection_t output) -> void {
    convert_from_to(circuit, output, SegmentPointType::input, SegmentPointType::output);
}

auto convert_to_inputs(CircuitData& circuit, const wire_connections_t& outputs) -> void {
    for (const auto output : outputs) {
        convert_to_input(circuit, output);
    }
}

auto convert_wires_at_outputs_to_inputs(CircuitData& circuit,
                                        const logicitem_id_t logicitem_id) -> void {
    const auto data = to_layout_calculation_data(circuit.layout, logicitem_id);

    auto result = find_convertible_wire_inputs(circuit, data);

    // we assume there will be no collision at this point
    if (result.any_collisions) [[unlikely]] {
        throw std::runtime_error("inserted logic item is colliding");
    }

    convert_to_inputs(circuit, result.convertible_inputs);
}

auto convert_wires_at_outputs_to_outputs(CircuitData& circuit,
                                         const logicitem_id_t logicitem_id) -> void {
    const auto data = to_layout_calculation_data(circuit.layout, logicitem_id);

    for (auto info : output_locations(data)) {
        if (const auto entry = circuit.index.wire_input_index().find(info.position)) {
            const auto connection = wire_connection_t {info.position, entry->segment};
            convert_to_output(circuit, connection);
        }
    }
}

//
// Logic Item Colliding
//

namespace {

auto _any_logicitem_inputs_colliding(const LayoutIndex& index,
                                     const layout_calculation_data_t& data) -> bool {
    const auto compatible = [&](simple_input_info_t info) -> bool {
        const auto entry = index.wire_output_index().find(info.position);
        return !entry || orientations_compatible(info.orientation, entry->orientation);
    };

    return !std::ranges::all_of(input_locations(data), compatible);
}

auto _any_logicitem_outputs_colliding(const CircuitData& circuit,
                                      const layout_calculation_data_t& data) -> bool {
    return find_convertible_wire_inputs(circuit, data).any_collisions;
}

}  // namespace

auto is_logicitem_colliding(const CircuitData& circuit,
                            const layout_calculation_data_t& data) -> bool {
    return circuit.index.collision_index().is_colliding(data) ||
           _any_logicitem_inputs_colliding(circuit.index, data) ||
           _any_logicitem_outputs_colliding(circuit, data);
}

auto is_logicitem_colliding(const CircuitData& circuit, const logicitem_id_t logicitem_id)
    -> bool {
    const auto data = to_layout_calculation_data(circuit.layout, logicitem_id);
    return is_logicitem_colliding(circuit, data);
}

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim
