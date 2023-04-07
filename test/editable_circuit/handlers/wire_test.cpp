#include "./test_helpers.h"
#include "editable_circuit/handlers.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

//
// add_line_segment
//

TEST(EditableCircuitHandlerWire, AddTempSegment) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();
    auto &schematic = circuit.schematic();
    auto &layout = circuit.layout();

    auto setup = HandlerSetup {circuit};
    add_wire_segment(setup.state, line_t {point_t {0, 0}, {10, 0}},
                     InsertionMode::temporary);

    setup.validate();

    // circuit
    assert_element_count(circuit, 2);
    {
        const auto id_0 = element_id_t {0};
        const auto &tree_0 = layout.segment_tree(id_0);

        ASSERT_EQ(schematic.element(id_0).is_wire(), true);
        ASSERT_EQ(layout.display_state(id_0), display_state_t::new_temporary);
        ASSERT_EQ(tree_0.segment_count(), 1);

        const auto line = ordered_line_t {point_t {0, 0}, {10, 0}};
        ASSERT_EQ(tree_0.segment_line(0), line);
    }
    {
        const auto id_1 = element_id_t {1};
        const auto &tree_1 = layout.segment_tree(id_1);

        ASSERT_EQ(schematic.element(id_1).is_wire(), true);
        ASSERT_EQ(layout.display_state(id_1), display_state_t::new_colliding);
        ASSERT_EQ(tree_1.segment_count(), 0);
    }

    // messages
}

}  // namespace logicsim