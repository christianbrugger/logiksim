#include "./test_helpers.h"
#include "editable_circuit/handlers.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

auto add_test_wire(Circuit &circuit, display_state_t display_state,
                   SegmentPointType point_type, std::span<const ordered_line_t> lines) {
    auto &schematic = circuit.schematic();
    auto &layout = circuit.layout();

    const auto element_id = schematic.add_element(
        Schematic::NewElementData {.element_type = ElementType::wire});
    layout.add_line_tree(display_state);

    auto &m_tree = layout.modifyable_segment_tree(element_id);
    for (const auto line : lines) {
        m_tree.add_segment(segment_info_t {
            .line = line,
            .p0_type = point_type,
            .p1_type = point_type,
            .p0_connection_id = null_connection,
            .p1_connection_id = null_connection,
        });
    }
}

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
    const auto m0
        = Message {SegmentCreated {segment_t {element_id_t {0}, segment_index_t {0}}}};
    ASSERT_EQ(setup.recorder.messages().size(), 1);
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
}

//
// Change Mode  -  Temporary => Colliding
//

TEST(EditableCircuitHandlerWire, TempToColliding) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();
    auto &schematic = circuit.schematic();
    auto &layout = circuit.layout();

    add_test_wire(circuit, new_temporary, SegmentPointType::shadow_point,
                  std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    add_test_wire(circuit, new_colliding, SegmentPointType::shadow_point, {});
    add_test_wire(circuit, normal, SegmentPointType::colliding_point,
                  std::array {ordered_line_t {point_t {1, 0}, point_t {3, 0}}});

    auto segment_part = segment_part_t {
        segment_t {element_id_t {0}, segment_index_t {0}},
        part_t {offset_t {0}, offset_t {10}},
    };

    auto setup = HandlerSetup {circuit};
    change_wire_insertion_mode(setup.state, segment_part, InsertionMode::collisions);

    setup.validate();

    // circuit
    assert_element_count(circuit, 3);
    {
        const auto element_id = element_id_t {0};
        const auto &tree = layout.segment_tree(element_id);

        ASSERT_EQ(schematic.element(element_id).is_wire(), true);
        ASSERT_EQ(layout.display_state(element_id), new_temporary);
        ASSERT_EQ(tree.segment_count(), 0);
    }
    {
        const auto element_id = element_id_t {1};
        const auto &tree = layout.segment_tree(element_id);

        ASSERT_EQ(schematic.element(element_id).is_wire(), true);
        ASSERT_EQ(layout.display_state(element_id), new_colliding);
        ASSERT_EQ(tree.segment_count(), 1);

        const auto line = ordered_line_t {point_t {0, 0}, {10, 0}};
        ASSERT_EQ(tree.segment_line(0), line);
    }
    {
        const auto element_id = element_id_t {2};
        const auto &tree = layout.segment_tree(element_id);

        ASSERT_EQ(schematic.element(element_id).is_wire(), true);
        ASSERT_EQ(layout.display_state(element_id), normal);
        ASSERT_EQ(tree.segment_count(), 1);

        const auto line = ordered_line_t {point_t {1, 0}, {3, 0}};
        ASSERT_EQ(tree.segment_line(0), line);
    }

    // messages
    const auto m0 = Message {SegmentIdUpdated {
        .new_segment = segment_t {element_id_t {1}, segment_index_t {0}},
        .old_segment = segment_t {element_id_t {0}, segment_index_t {0}},
    }};

    ASSERT_EQ(setup.recorder.messages().size(), 1);
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
}

TEST(EditableCircuitHandlerWire, TempToCollidingPartialOneSide) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();
    auto &schematic = circuit.schematic();
    auto &layout = circuit.layout();

    add_test_wire(circuit, new_temporary, SegmentPointType::shadow_point,
                  std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    add_test_wire(circuit, new_colliding, SegmentPointType::shadow_point, {});
    add_test_wire(circuit, normal, SegmentPointType::colliding_point,
                  std::array {ordered_line_t {point_t {1, 0}, point_t {3, 0}}});

    auto segment_part = segment_part_t {
        segment_t {element_id_t {0}, segment_index_t {0}},
        part_t {offset_t {0}, offset_t {5}},
    };

    auto setup = HandlerSetup {circuit};
    change_wire_insertion_mode(setup.state, segment_part, InsertionMode::collisions);

    setup.validate();

    // circuit
    assert_element_count(circuit, 3);
    {
        const auto element_id = element_id_t {0};
        const auto &tree = layout.segment_tree(element_id);

        ASSERT_EQ(schematic.element(element_id).is_wire(), true);
        ASSERT_EQ(layout.display_state(element_id), new_temporary);
        ASSERT_EQ(tree.segment_count(), 1);

        const auto line = ordered_line_t {point_t {5, 0}, {10, 0}};
        ASSERT_EQ(tree.segment_line(0), line);
    }
    {
        const auto element_id = element_id_t {1};
        const auto &tree = layout.segment_tree(element_id);

        ASSERT_EQ(schematic.element(element_id).is_wire(), true);
        ASSERT_EQ(layout.display_state(element_id), new_colliding);
        ASSERT_EQ(tree.segment_count(), 1);

        const auto line = ordered_line_t {point_t {0, 0}, {5, 0}};
        ASSERT_EQ(tree.segment_line(0), line);
    }
    {
        const auto element_id = element_id_t {2};
        const auto &tree = layout.segment_tree(element_id);

        ASSERT_EQ(schematic.element(element_id).is_wire(), true);
        ASSERT_EQ(layout.display_state(element_id), normal);
        ASSERT_EQ(tree.segment_count(), 1);

        const auto line = ordered_line_t {point_t {1, 0}, {3, 0}};
        ASSERT_EQ(tree.segment_line(0), line);
    }

    // messages
    const auto m0
        = Message {SegmentCreated {segment_t {element_id_t {1}, segment_index_t {0}}}};
    const auto m1 = Message {SegmentPartMoved {
        .segment_part_destination
        = segment_part_t {segment_t {element_id_t {1}, segment_index_t {0}},
                          part_t {offset_t {0}, offset_t {5}}},
        .segment_part_source
        = segment_part_t {segment_t {element_id_t {0}, segment_index_t {0}},
                          part_t {offset_t {0}, offset_t {5}}},
    }};
    ASSERT_EQ(setup.recorder.messages().size(), 2);
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
    ASSERT_EQ(setup.recorder.messages().at(1), m1);
}

TEST(EditableCircuitHandlerWire, TempToCollidingPartialMiddle) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();
    auto &schematic = circuit.schematic();
    auto &layout = circuit.layout();

    add_test_wire(circuit, new_temporary, SegmentPointType::shadow_point,
                  std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    add_test_wire(circuit, new_colliding, SegmentPointType::shadow_point, {});
    add_test_wire(circuit, normal, SegmentPointType::colliding_point,
                  std::array {ordered_line_t {point_t {1, 0}, point_t {3, 0}}});

    auto segment_part = segment_part_t {
        segment_t {element_id_t {0}, segment_index_t {0}},
        part_t {offset_t {2}, offset_t {5}},
    };

    auto setup = HandlerSetup {circuit};
    change_wire_insertion_mode(setup.state, segment_part, InsertionMode::collisions);

    setup.validate();

    // circuit
    assert_element_count(circuit, 3);
    {
        const auto element_id = element_id_t {0};
        const auto &tree = layout.segment_tree(element_id);

        ASSERT_EQ(schematic.element(element_id).is_wire(), true);
        ASSERT_EQ(layout.display_state(element_id), new_temporary);
        ASSERT_EQ(tree.segment_count(), 2);

        const auto line0 = ordered_line_t {point_t {0, 0}, {2, 0}};
        const auto line1 = ordered_line_t {point_t {5, 0}, {10, 0}};
        ASSERT_EQ(tree.segment_line(0), line0);
        ASSERT_EQ(tree.segment_line(1), line1);
    }
    {
        const auto element_id = element_id_t {1};
        const auto &tree = layout.segment_tree(element_id);

        ASSERT_EQ(schematic.element(element_id).is_wire(), true);
        ASSERT_EQ(layout.display_state(element_id), new_colliding);
        ASSERT_EQ(tree.segment_count(), 1);

        const auto line = ordered_line_t {point_t {2, 0}, {5, 0}};
        ASSERT_EQ(tree.segment_line(0), line);
    }
    {
        const auto element_id = element_id_t {2};
        const auto &tree = layout.segment_tree(element_id);

        ASSERT_EQ(schematic.element(element_id).is_wire(), true);
        ASSERT_EQ(layout.display_state(element_id), normal);
        ASSERT_EQ(tree.segment_count(), 1);

        const auto line = ordered_line_t {point_t {1, 0}, {3, 0}};
        ASSERT_EQ(tree.segment_line(0), line);
    }

    // messages
    const auto m0
        = Message {SegmentCreated {segment_t {element_id_t {0}, segment_index_t {1}}}};
    const auto m1 = Message {SegmentPartMoved {
        .segment_part_destination
        = segment_part_t {segment_t {element_id_t {0}, segment_index_t {1}},
                          part_t {offset_t {0}, offset_t {5}}},
        .segment_part_source
        = segment_part_t {segment_t {element_id_t {0}, segment_index_t {0}},
                          part_t {offset_t {5}, offset_t {10}}},
    }};
    const auto m2
        = Message {SegmentCreated {segment_t {element_id_t {1}, segment_index_t {0}}}};
    const auto m3 = Message {SegmentPartMoved {
        .segment_part_destination
        = segment_part_t {segment_t {element_id_t {1}, segment_index_t {0}},
                          part_t {offset_t {0}, offset_t {3}}},
        .segment_part_source
        = segment_part_t {segment_t {element_id_t {0}, segment_index_t {0}},
                          part_t {offset_t {2}, offset_t {5}}},
    }};
    ASSERT_EQ(setup.recorder.messages().size(), 4);
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
    ASSERT_EQ(setup.recorder.messages().at(1), m1);
    ASSERT_EQ(setup.recorder.messages().at(2), m2);
    ASSERT_EQ(setup.recorder.messages().at(3), m3);
}

//
// Change Mode  -  Temporary => Valid
//

TEST(EditableCircuitHandlerWire, TempToValid) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();
    auto &schematic = circuit.schematic();
    auto &layout = circuit.layout();

    add_test_wire(circuit, new_temporary, SegmentPointType::shadow_point,
                  std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    add_test_wire(circuit, new_colliding, SegmentPointType::shadow_point, {});

    auto segment_part = segment_part_t {
        segment_t {element_id_t {0}, segment_index_t {0}},
        part_t {offset_t {0}, offset_t {10}},
    };

    auto setup = HandlerSetup {circuit};
    change_wire_insertion_mode(setup.state, segment_part, InsertionMode::collisions);
    setup.validate();

    // circuit
    assert_element_count(circuit, 3);
    {
        const auto element_id = element_id_t {0};
        const auto &tree = layout.segment_tree(element_id);

        ASSERT_EQ(schematic.element(element_id).is_wire(), true);
        ASSERT_EQ(layout.display_state(element_id), new_temporary);
        ASSERT_EQ(tree.segment_count(), 0);
    }
    {
        const auto element_id = element_id_t {1};
        const auto &tree = layout.segment_tree(element_id);

        ASSERT_EQ(schematic.element(element_id).is_wire(), true);
        ASSERT_EQ(layout.display_state(element_id), new_colliding);
        ASSERT_EQ(tree.segment_count(), 0);
    }
    {
        const auto element_id = element_id_t {2};
        const auto &tree = layout.segment_tree(element_id);

        ASSERT_EQ(schematic.element(element_id).is_wire(), true);
        ASSERT_EQ(layout.display_state(element_id), new_valid);
        ASSERT_EQ(tree.segment_count(), 1);

        const auto line = ordered_line_t {point_t {0, 0}, {10, 0}};
        ASSERT_EQ(tree.segment_line(0), line);
    }

    // messages
    const auto m0 = Message {SegmentIdUpdated {
        .new_segment = segment_t {element_id_t {2}, segment_index_t {0}},
        .old_segment = segment_t {element_id_t {0}, segment_index_t {0}},
    }};
    const auto segment1 = segment_t {element_id_t {2}, segment_index_t {0}};
    const auto m1 = Message {SegmentInserted {
        .segment = segment1,
        .segment_info = get_segment_info(circuit, segment1),
    }};

    ASSERT_EQ(setup.recorder.messages().size(), 2);
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
    ASSERT_EQ(setup.recorder.messages().at(1), m1);
}

}  // namespace logicsim