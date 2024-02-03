#ifndef LOGIKSIM_TEST_EDITABLE_CIRCUIT_HANDLERS_TEST_HELPERS_H
#define LOGIKSIM_TEST_EDITABLE_CIRCUIT_HANDLERS_TEST_HELPERS_H

#include "component/editable_circuit/modifier.h"
#include "layout.h"
#include "vocabulary/logicitem_definition.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

inline auto add_and_element(Layout &layout, display_state_t display_type,
                            connection_count_t input_count = connection_count_t {3},
                            point_t position = point_t {0, 0}) -> logicitem_id_t {
    const auto definition = LogicItemDefinition {
        .logicitem_type = LogicItemType::and_element,

        .input_count = input_count,
        .output_count = connection_count_t {1},
        .orientation = orientation_t::right,
    };
    return layout.logic_items().add(definition, position, display_type);
}

inline auto assert_logicitem_count(const Layout &layout, std::size_t count) -> void {
    ASSERT_EQ(layout.logic_items().size(), count);
}

inline auto assert_logicitem_equal(
    const Layout &layout, logicitem_id_t logicitem_id,
    connection_count_t input_count = connection_count_t {3},
    point_t position = point_t {0, 0}) -> void {
    ASSERT_EQ(layout.logic_items().input_count(logicitem_id), input_count);
    ASSERT_EQ(layout.logic_items().position(logicitem_id), position);
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

inline auto get_display_state(const Modifier &modifier, logicitem_id_t logicitem_id)
    -> display_state_t {
    return modifier.circuit_data().layout.logic_items().display_state(logicitem_id);
}

inline auto assert_wire_count(const Modifier &modifier, std::size_t count) -> void {
    ASSERT_EQ(modifier.circuit_data().layout.wires().size(), count);
}

inline auto get_segment_tree(const Modifier &modifier, wire_id_t wire_id)
    -> const SegmentTree & {
    return modifier.circuit_data().layout.wires().segment_tree(wire_id);
}

}  // namespace editable_circuit

}  // namespace logicsim

#endif