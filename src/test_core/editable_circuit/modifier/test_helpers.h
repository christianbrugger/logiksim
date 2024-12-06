#ifndef LOGIKSIM_TEST_EDITABLE_CIRCUIT_HANDLERS_TEST_HELPERS_H
#define LOGIKSIM_TEST_EDITABLE_CIRCUIT_HANDLERS_TEST_HELPERS_H

#include "core/component/editable_circuit/modifier.h"
#include "core/editable_circuit.h"
#include "core/format/struct.h"
#include "core/layout.h"
#include "core/vocabulary/logicitem_definition.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

//
// Construction
//

namespace editable_circuit {

[[nodiscard]] inline auto get_modifier(Layout layout_ = Layout {}) -> Modifier {
    const auto modifier = Modifier {
        std::move(layout_),
        ModifierConfig {
            .store_messages = false,
            .validate_messages = true,
        },
    };
    Expects(modifier.circuit_data().message_validator.has_value());
    Expects(!modifier.circuit_data().messages.has_value());
    Expects(is_valid(modifier));
    return modifier;
}

[[nodiscard]] inline auto get_modifier_with_history(Layout layout_ = Layout {})
    -> Modifier {
    auto modifier = get_modifier(std::move(layout_));
    modifier.enable_history();
    return modifier;
}

[[nodiscard]] inline auto get_logging_modifier(Layout layout_ = Layout {}) -> Modifier {
    const auto modifier = Modifier {
        std::move(layout_),
        ModifierConfig {
            .store_messages = true,
            .validate_messages = true,
        },
    };
    Expects(modifier.circuit_data().message_validator.has_value());
    Expects(modifier.circuit_data().messages.has_value());
    Expects(is_valid(modifier));
    return modifier;
}

}  // namespace editable_circuit

[[nodiscard]] inline auto get_editable_circuit(Layout layout_ = Layout {})
    -> EditableCircuit {
    const auto editable_circuit = EditableCircuit {
        std::move(layout_),
        EditableCircuit::Config {
            .store_messages = false,
            .validate_messages = true,
        },
    };
    Expects(editable_circuit.modifier().circuit_data().message_validator.has_value());
    Expects(!editable_circuit.modifier().circuit_data().messages.has_value());
    Expects(is_valid(editable_circuit));
    return editable_circuit;
}

[[nodiscard]] inline auto get_logging_editable_circuit(Layout layout_ = Layout {})
    -> EditableCircuit {
    const auto editable_circuit = EditableCircuit {
        std::move(layout_),
        EditableCircuit::Config {
            .store_messages = true,
            .validate_messages = true,
        },
    };
    Expects(editable_circuit.modifier().circuit_data().message_validator.has_value());
    Expects(editable_circuit.modifier().circuit_data().messages.has_value());
    Expects(is_valid(editable_circuit));
    return editable_circuit;
}

//
// Add Elements
//

inline auto add_and_element(Layout &layout, display_state_t display_type,
                            connection_count_t input_count = connection_count_t {3},
                            point_t position = point_t {0, 0}) -> logicitem_id_t {
    auto definition = LogicItemDefinition {
        .logicitem_type = LogicItemType::and_element,

        .input_count = input_count,
        .output_count = connection_count_t {1},
        .orientation = orientation_t::right,
    };
    return layout.logicitems().add(std::move(definition), position, display_type);
}

inline auto assert_logicitem_count(const Layout &layout, std::size_t count) -> void {
    ASSERT_EQ(layout.logicitems().size(), count);
}

inline auto assert_logicitem_equal(
    const Layout &layout, logicitem_id_t logicitem_id,
    connection_count_t input_count = connection_count_t {3},
    point_t position = point_t {0, 0}) -> void {
    ASSERT_EQ(layout.logicitems().input_count(logicitem_id), input_count);
    ASSERT_EQ(layout.logicitems().position(logicitem_id), position);
}

namespace editable_circuit {

inline auto assert_logicitem_count(const Modifier &modifier, std::size_t count) -> void {
    assert_logicitem_count(modifier.circuit_data().layout, count);
}

inline auto assert_logicitem_equal(
    const Modifier &modifier, logicitem_id_t logicitem_id,
    connection_count_t input_count = connection_count_t {3},
    point_t position = point_t {0, 0}) -> void {
    assert_logicitem_equal(modifier.circuit_data().layout, logicitem_id, input_count,
                           position);
}

inline auto get_display_state(const Modifier &modifier,
                              logicitem_id_t logicitem_id) -> display_state_t {
    return modifier.circuit_data().layout.logicitems().display_state(logicitem_id);
}

inline auto assert_wire_count(const Modifier &modifier, std::size_t count) -> void {
    ASSERT_EQ(modifier.circuit_data().layout.wires().size(), count);
}

inline auto get_segment_tree(const Modifier &modifier,
                             wire_id_t wire_id) -> const SegmentTree & {
    return modifier.circuit_data().layout.wires().segment_tree(wire_id);
}

//
// Add Wire
//

inline auto add_to_wire(Layout &layout, wire_id_t wire_id, SegmentPointType point_type,
                        ordered_line_t line) -> segment_index_t {
    auto &m_tree = layout.wires().modifiable_segment_tree(wire_id);

    return m_tree.add_segment(segment_info_t {
        .line = line,
        .p0_type = point_type,
        .p1_type = point_type,
    });
}

inline auto add_to_wire(Layout &layout, wire_id_t wire_id, SegmentPointType point_type,
                        std::span<const ordered_line_t> lines) -> void {
    auto &m_tree = layout.wires().modifiable_segment_tree(wire_id);

    for (const auto &line : lines) {
        m_tree.add_segment(segment_info_t {
            .line = line,
            .p0_type = point_type,
            .p1_type = point_type,
        });
    }
}

inline auto add_test_wire(Layout &layout, SegmentPointType point_type,
                          std::span<const ordered_line_t> lines) -> void {
    const auto wire_id = layout.wires().add_wire();
    add_to_wire(layout, wire_id, point_type, lines);
}

}  // namespace editable_circuit

}  // namespace logicsim

#endif
