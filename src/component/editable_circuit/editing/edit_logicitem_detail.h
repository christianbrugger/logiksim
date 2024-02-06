#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_LOGICITEM_DETAIL_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_LOGICITEM_DETAIL_H

#include "format/struct.h"
#include "vocabulary/point.h"
#include "vocabulary/segment.h"
#include "vocabulary/segment_point_type.h"

#include <folly/small_vector.h>

#include <compare>

namespace logicsim {

struct logicitem_id_t;
struct layout_calculation_data_t;

class Layout;
class LayoutIndex;

namespace editable_circuit {

struct CircuitData;

//
// Wire Connections
//

struct wire_connection_t {
    point_t position;
    segment_t segment;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const wire_connection_t&) const -> bool = default;
    [[nodiscard]] auto operator<=>(const wire_connection_t&) const = default;
};

using policy = folly::small_vector_policy::policy_size_type<uint32_t>;
using wire_connections_t = folly::small_vector<wire_connection_t, 3, policy>;

static_assert(sizeof(wire_connection_t) == 12);
static_assert(sizeof(wire_connections_t) == 40);

[[nodiscard]] auto has_duplicate_wire_ids(wire_connections_t connections) -> bool;

//
// Check Convertible Inputs
//

[[nodiscard]] auto is_convertible_to_input(const Layout& layout, wire_id_t wire_id)
    -> bool;
[[nodiscard]] auto all_convertible_to_input(const Layout& layout,
                                            const wire_connections_t& connections)
    -> bool;

struct convertible_inputs_result_t {
    wire_connections_t convertible_inputs {};
    bool any_collisions {false};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const convertible_inputs_result_t&) const
        -> bool = default;
};

[[nodiscard]] auto find_convertible_wire_inputs(const CircuitData& circuit,
                                                const layout_calculation_data_t& data)
    -> convertible_inputs_result_t;

//
// Convert Inputs / Outputs
//

auto convert_from_to(CircuitData& circuit, wire_connection_t output,
                     SegmentPointType from_type, SegmentPointType to_type) -> void;

auto convert_to_input(CircuitData& circuit, wire_connection_t output) -> void;

auto convert_to_inputs(CircuitData& circuit, wire_connections_t outputs) -> void;

auto convert_to_output(CircuitData& circuit, wire_connection_t output) -> void;

auto convert_wires_at_outputs_to_inputs(CircuitData& circuit,
                                        const logicitem_id_t logicitem_id) -> void;

auto convert_wires_at_outputs_to_outputs(CircuitData& circuit,
                                         const logicitem_id_t logicitem_id) -> void;

//
// Logic Item Colliding
//

auto is_logicitem_colliding(CircuitData& circuit, const layout_calculation_data_t& data)
    -> bool;

auto is_logicitem_colliding(CircuitData& circuit, const logicitem_id_t logicitem_id)
    -> bool;

}  // namespace editable_circuit

}  // namespace logicsim

#endif
