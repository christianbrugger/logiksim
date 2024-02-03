#include "./test_helpers.h"
#include "layout_message.h"
#include "vocabulary/logicitem_definition.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

/*

namespace {

auto add_to_wire(Layout &layout, wire_id_t wire_id, SegmentPointType point_type,
                 std::span<const ordered_line_t> lines) {
    auto &m_tree = layout.wires().modifiable_segment_tree(wire_id);

    for (const auto &line : lines) {
        m_tree.add_segment(segment_info_t {
            .line = line,
            .p0_type = point_type,
            .p1_type = point_type,
        });
    }
}

auto add_test_wire(Layout &layout, SegmentPointType point_type,
                   std::span<const ordered_line_t> lines) {
    const auto wire_id = layout.wires().add_wire();
    add_to_wire(layout, wire_id, point_type, lines);
}

inline auto part(offset_t::value_type begin, offset_t::value_type end) -> part_t {
    return part_t {offset_t {begin}, offset_t {end}};
}

}  // namespace

//
// add_line_segment
//

TEST(EditableCircuitHandlerWire, AddTempSegment) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto setup = HandlerSetup {layout};
    add_wire_segment(setup.state, ordered_line_t {point_t {0, 0}, point_t {10, 0}},
                     InsertionMode::temporary);

    // layout
    ASSERT_EQ(layout.wires().size(), 1);
    {
        const auto id_0 = wire_id_t {0};
        const auto &tree_0 = layout.wires().segment_tree(id_0);

        ASSERT_EQ(is_temporary(id_0), true);
        ASSERT_EQ(tree_0.size(), 1);

        const auto line = ordered_line_t {point_t {0, 0}, point_t {10, 0}};
        ASSERT_EQ(tree_0.line(segment_index_t {0}), line);
    }
    {
        const auto id_1 = wire_id_t {1};
        const auto &tree_1 = layout.wires().segment_tree(id_1);

        ASSERT_EQ(is_colliding(id_1), true);
        ASSERT_EQ(tree_1.size(), 0);
    }

    // messages
    const auto m0 =
        Message {SegmentCreated {segment_t {wire_id_t {0}, segment_index_t {0}}}};
    ASSERT_EQ(setup.recorder.messages().size(), 1);
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
}

//
// Change Mode  -  Temporary => Colliding
//

TEST(EditableCircuitHandlerWire, TempToColliding) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    add_to_wire(layout, colliding_wire_id, SegmentPointType::shadow_point, {});
    add_test_wire(layout, SegmentPointType::output,
                  std::array {ordered_line_t {point_t {1, 0}, point_t {3, 0}}});

    auto segment_part = segment_part_t {
        segment_t {wire_id_t {0}, segment_index_t {0}},
        part_t {offset_t {0}, offset_t {10}},
    };

    auto setup = HandlerSetup {layout};
    change_wire_insertion_mode(setup.state, segment_part, InsertionMode::collisions);

    setup.validate();

    // layout
    ASSERT_EQ(layout.wires().size(), 3);
    {
        const auto wire_id = wire_id_t {0};
        const auto &tree = layout.wires().segment_tree(wire_id);

        ASSERT_EQ(is_temporary(wire_id), true);
        ASSERT_EQ(tree.size(), 0);
    }
    {
        const auto wire_id = wire_id_t {1};
        const auto &tree = layout.wires().segment_tree(wire_id);

        ASSERT_EQ(is_colliding(wire_id), true);
        ASSERT_EQ(tree.size(), 1);

        const auto line = ordered_line_t {point_t {0, 0}, point_t {10, 0}};
        ASSERT_EQ(tree.line(segment_index_t {0}), line);
    }
    {
        const auto wire_id = wire_id_t {2};
        const auto &tree = layout.wires().segment_tree(wire_id);

        ASSERT_EQ(to_display_state(wire_id), normal);
        ASSERT_EQ(tree.size(), 1);

        const auto line = ordered_line_t {point_t {1, 0}, point_t {3, 0}};
        ASSERT_EQ(tree.line(segment_index_t {0}), line);
    }

    // messages
    const auto m0 = Message {SegmentIdUpdated {
        .new_segment = segment_t {wire_id_t {1}, segment_index_t {0}},
        .old_segment = segment_t {wire_id_t {0}, segment_index_t {0}},
    }};

    ASSERT_EQ(setup.recorder.messages().size(), 1);
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
}

TEST(EditableCircuitHandlerWire, TempToCollidingPartialOneSide) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    add_to_wire(layout, colliding_wire_id, SegmentPointType::shadow_point, {});
    add_test_wire(layout, SegmentPointType::output,
                  std::array {ordered_line_t {point_t {1, 0}, point_t {3, 0}}});

    auto segment_part =
        segment_part_t {segment_t {wire_id_t {0}, segment_index_t {0}}, part(0, 5)};

    auto setup = HandlerSetup {layout};
    change_wire_insertion_mode(setup.state, segment_part, InsertionMode::collisions);

    setup.validate();

    // layout
    ASSERT_EQ(layout.wires().size(), 3);
    {
        const auto wire_id = wire_id_t {0};
        const auto &tree = layout.wires().segment_tree(wire_id);

        ASSERT_EQ(is_temporary(wire_id), true);
        ASSERT_EQ(tree.size(), 1);

        const auto line = ordered_line_t {point_t {5, 0}, point_t {10, 0}};
        ASSERT_EQ(tree.line(segment_index_t {0}), line);
    }
    {
        const auto wire_id = wire_id_t {1};
        const auto &tree = layout.wires().segment_tree(wire_id);

        ASSERT_EQ(is_colliding(wire_id), true);
        ASSERT_EQ(tree.size(), 1);

        const auto line = ordered_line_t {point_t {0, 0}, point_t {5, 0}};
        ASSERT_EQ(tree.line(segment_index_t {0}), line);
    }
    {
        const auto wire_id = wire_id_t {2};
        const auto &tree = layout.wires().segment_tree(wire_id);

        ASSERT_EQ(to_display_state(wire_id), normal);
        ASSERT_EQ(tree.size(), 1);

        const auto line = ordered_line_t {point_t {1, 0}, point_t {3, 0}};
        ASSERT_EQ(tree.line(segment_index_t {0}), line);
    }

    // messages
    const auto m0 =
        Message {SegmentCreated {segment_t {wire_id_t {1}, segment_index_t {0}}}};
    const auto m1 = Message {SegmentPartMoved {
        .segment_part_destination =
            segment_part_t {segment_t {wire_id_t {1}, segment_index_t {0}}, part(0, 5)},
        .segment_part_source =
            segment_part_t {segment_t {wire_id_t {0}, segment_index_t {0}}, part(0, 5)},
    }};
    const auto m2 = Message {SegmentPartMoved {
        .segment_part_destination =
            segment_part_t {segment_t {wire_id_t {0}, segment_index_t {0}}, part(0, 5)},
        .segment_part_source =
            segment_part_t {segment_t {wire_id_t {0}, segment_index_t {0}}, part(5, 10)},
    }};
    ASSERT_EQ(setup.recorder.messages().size(), 3);
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
    ASSERT_EQ(setup.recorder.messages().at(1), m1);
    ASSERT_EQ(setup.recorder.messages().at(2), m2);
}

TEST(EditableCircuitHandlerWire, TempToCollidingPartialMiddle) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    add_to_wire(layout, colliding_wire_id, SegmentPointType::shadow_point, {});
    add_test_wire(layout, SegmentPointType::output,
                  std::array {ordered_line_t {point_t {1, 0}, point_t {3, 0}}});

    auto segment_part = segment_part_t {
        segment_t {wire_id_t {0}, segment_index_t {0}},
        part_t {offset_t {2}, offset_t {5}},
    };

    auto setup = HandlerSetup {layout};
    change_wire_insertion_mode(setup.state, segment_part, InsertionMode::collisions);

    setup.validate();

    // layout
    ASSERT_EQ(layout.wires().size(), 3);
    {
        const auto wire_id = wire_id_t {0};
        const auto &tree = layout.wires().segment_tree(wire_id);

        ASSERT_EQ(is_temporary(wire_id), true);
        ASSERT_EQ(tree.size(), 2);

        const auto line0 = ordered_line_t {point_t {0, 0}, point_t {2, 0}};
        const auto line1 = ordered_line_t {point_t {5, 0}, point_t {10, 0}};
        ASSERT_EQ(tree.line(segment_index_t {0}), line0);
        ASSERT_EQ(tree.line(segment_index_t {1}), line1);
    }
    {
        const auto wire_id = wire_id_t {1};
        const auto &tree = layout.wires().segment_tree(wire_id);

        ASSERT_EQ(is_colliding(wire_id), true);
        ASSERT_EQ(tree.size(), 1);

        const auto line = ordered_line_t {point_t {2, 0}, point_t {5, 0}};
        ASSERT_EQ(tree.line(segment_index_t {0}), line);
    }
    {
        const auto wire_id = wire_id_t {2};
        const auto &tree = layout.wires().segment_tree(wire_id);

        ASSERT_EQ(to_display_state(wire_id), normal);
        ASSERT_EQ(tree.size(), 1);

        const auto line = ordered_line_t {point_t {1, 0}, point_t {3, 0}};
        ASSERT_EQ(tree.line(segment_index_t {0}), line);
    }

    // messages
    const auto m0 =
        Message {SegmentCreated {segment_t {wire_id_t {0}, segment_index_t {1}}}};
    const auto m1 =
        Message {SegmentCreated {segment_t {wire_id_t {1}, segment_index_t {0}}}};
    const auto m2 = Message {SegmentPartMoved {
        .segment_part_destination =
            segment_part_t {segment_t {wire_id_t {0}, segment_index_t {1}},
                            part_t {offset_t {0}, offset_t {5}}},
        .segment_part_source =
            segment_part_t {segment_t {wire_id_t {0}, segment_index_t {0}},
                            part_t {offset_t {5}, offset_t {10}}},
    }};
    const auto m3 = Message {SegmentPartMoved {
        .segment_part_destination =
            segment_part_t {segment_t {wire_id_t {1}, segment_index_t {0}},
                            part_t {offset_t {0}, offset_t {3}}},
        .segment_part_source =
            segment_part_t {segment_t {wire_id_t {0}, segment_index_t {0}},
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
    auto layout = Layout {};

    add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    add_to_wire(layout, colliding_wire_id, SegmentPointType::shadow_point, {});

    auto segment_part = segment_part_t {
        segment_t {wire_id_t {0}, segment_index_t {0}},
        part_t {offset_t {0}, offset_t {10}},
    };

    const auto info_0 = get_segment_info(layout, segment_part.segment);

    auto setup = HandlerSetup {layout};
    change_wire_insertion_mode(setup.state, segment_part, InsertionMode::collisions);
    setup.validate();

    // layout
    ASSERT_EQ(layout.wires().size(), 3);
    {
        const auto wire_id = wire_id_t {0};
        const auto &tree = layout.wires().segment_tree(wire_id);

        ASSERT_EQ(is_temporary(wire_id), true);
        ASSERT_EQ(tree.size(), 0);
    }
    {
        const auto wire_id = wire_id_t {1};
        const auto &tree = layout.wires().segment_tree(wire_id);

        ASSERT_EQ(is_colliding(wire_id), true);
        ASSERT_EQ(tree.size(), 0);
    }
    {
        const auto wire_id = wire_id_t {2};
        const auto &tree = layout.wires().segment_tree(wire_id);

        ASSERT_EQ(to_display_state(wire_id), normal);
        ASSERT_EQ(tree.size(), 1);

        const auto line = ordered_line_t {point_t {0, 0}, point_t {10, 0}};
        ASSERT_EQ(tree.line(segment_index_t {0}), line);

        ASSERT_EQ(tree.valid_parts(segment_index_t {0}).size(), 1);
        ASSERT_EQ(*tree.valid_parts(segment_index_t {0}).begin(), part(0, 10));
    }

    // messages
    const auto m0 = Message {SegmentIdUpdated {
        .new_segment = segment_t {wire_id_t {2}, segment_index_t {0}},
        .old_segment = segment_t {wire_id_t {0}, segment_index_t {0}},
    }};

    const auto segment1 = segment_t {wire_id_t {2}, segment_index_t {0}};
    const auto m1 = Message {SegmentInserted {
        .segment = segment1,
        .segment_info = info_0,
    }};

    auto info_1 = info_0;
    info_1.p0_type = SegmentPointType::output;

    auto info_2 = info_1;
    info_2.p1_type = SegmentPointType::output;

    const auto m2 = Message {InsertedEndPointsUpdated {
        .segment = segment1,
        .new_segment_info = info_1,
        .old_segment_info = info_0,
    }};
    const auto m3 = Message {InsertedEndPointsUpdated {
        .segment = segment1,
        .new_segment_info = info_2,
        .old_segment_info = info_1,
    }};

    ASSERT_EQ(setup.recorder.messages().size(), 4);
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
    ASSERT_EQ(setup.recorder.messages().at(1), m1);
    ASSERT_EQ(setup.recorder.messages().at(2), m2);
    ASSERT_EQ(setup.recorder.messages().at(3), m3);
}

//
// is_wire_position_representable
//

TEST(EditableCircuitHandlerWire, IsWirePositionRepresentable) {
    using namespace editable_circuit;
    auto layout = Layout {};

    const auto wire_id = wire_id_t {0};
    auto &m_tree = layout.wires().modifiable_segment_tree(wire_id);
    const auto segment_index = m_tree.add_segment(
        segment_info_t {.line = ordered_line_t {point_t {0, 0}, point_t {10, 0}}});

    const auto segment = segment_t {wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part(0, 10)};

    constexpr static auto overflow = int {grid_t::max()} + 100;

    ASSERT_EQ(is_wire_position_representable(layout, segment_part, 10, 10), true);
    ASSERT_EQ(is_wire_position_representable(layout, segment_part, -10, -10), true);

    ASSERT_EQ(is_wire_position_representable(layout, segment_part, overflow, 10), false);
    ASSERT_EQ(is_wire_position_representable(layout, segment_part, -overflow, 10), false);
    ASSERT_EQ(is_wire_position_representable(layout, segment_part, 0, overflow), false);
}

TEST(EditableCircuitHandlerWire, IsWirePositionRepresentablePart) {
    using namespace editable_circuit;
    auto layout = Layout {};

    auto p1_x = grid_t::max();

    const auto wire_id = wire_id_t {0};
    auto &m_tree = layout.wires().modifiable_segment_tree(wire_id);
    const auto segment_index = m_tree.add_segment(
        segment_info_t {.line = ordered_line_t {point_t {0, 0}, point_t {p1_x, 0}}});

    const auto segment = segment_t {wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part(0, 10)};
    const auto segment_full = segment_part_t {
        segment,
        m_tree.part(segment_index),
    };

    ASSERT_EQ(is_wire_position_representable(layout, segment_part, -10, -10), true);
    ASSERT_EQ(is_wire_position_representable(layout, segment_part, 10, 10), true);

    ASSERT_EQ(is_wire_position_representable(layout, segment_full, -10, -10), true);
    ASSERT_EQ(is_wire_position_representable(layout, segment_full, 10, 10), false);
}

//
// move_or_delete_wire
//

TEST(EditableCircuitHandlerWire, MoveOrDeleteWireMove) {
    using namespace editable_circuit::info_message;
    auto layout = Layout {};

    const auto line = ordered_line_t {point_t {0, 0}, point_t {10, 0}};
    const auto line_0 = ordered_line_t {point_t {100, 200}, point_t {110, 200}};

    const auto wire_id = wire_id_t {0};
    auto &m_tree = layout.wires().modifiable_segment_tree(wire_id);
    const auto segment_index = m_tree.add_segment(segment_info_t {.line = line});

    const auto segment_part_0 =
        segment_part_t {segment_t {wire_id, segment_index}, part(0, 10)};

    auto setup = SenderSetup {};

    auto segment_part = segment_part_0;
    move_or_delete_wire(layout, setup.sender, segment_part, 100, 200);

    ASSERT_EQ(layout.wires().size(), 1);
    ASSERT_EQ(is_temporary(wire_id_t {0}), true);

    ASSERT_EQ(segment_part, segment_part_0);

    const auto &tree = layout.wires().segment_tree(wire_id_t {0});
    ASSERT_EQ(tree.size(), 1);
    ASSERT_EQ(tree.line(segment_index_t {0}), line_0);

    // messages
    const auto m0 =
        Message {SegmentCreated {segment_t {wire_id_t {0}, segment_index_t {0}}}};
    ASSERT_EQ(setup.recorder.messages().size(), 1);
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
}

TEST(EditableCircuitHandlerWire, MoveOrDeleteWireMovePartialBegin) {
    using namespace editable_circuit::info_message;
    auto layout = Layout {};

    const auto line = ordered_line_t {point_t {0, 0}, point_t {10, 0}};
    const auto line_0 = ordered_line_t {point_t {5, 0}, point_t {10, 0}};
    const auto line_1 = ordered_line_t {point_t {100, 200}, point_t {105, 200}};

    const auto wire_id = wire_id_t {0};
    auto &m_tree = layout.wires().modifiable_segment_tree(wire_id);
    const auto segment_index = m_tree.add_segment(segment_info_t {.line = line});

    const auto segment_part_0 =
        segment_part_t {segment_t {wire_id, segment_index}, part(0, 5)};
    const auto segment_part_1 =
        segment_part_t {segment_t {wire_id, segment_index_t {1}}, part(0, 5)};

    auto setup = SenderSetup {};

    auto segment_part = segment_part_0;
    move_or_delete_wire(layout, setup.sender, segment_part, 100, 200);

    ASSERT_EQ(segment_part, segment_part_1);
    ASSERT_EQ(layout.wires().size(), 1);
    ASSERT_EQ(is_temporary(wire_id_t {0}), true);

    const auto &tree = layout.wires().segment_tree(wire_id_t {0});
    ASSERT_EQ(tree.size(), 2);
    ASSERT_EQ(tree.line(segment_index_t {0}), line_0);
    ASSERT_EQ(tree.line(segment_index_t {1}), line_1);

    // messages
    const auto m0 = Message {SegmentCreated {segment_part_1.segment}};
    const auto m1 = Message {SegmentPartMoved {
        .segment_part_destination = segment_part_1,
        .segment_part_source = segment_part_0,
    }};
    const auto m2 = Message {SegmentPartMoved {
        .segment_part_destination =
            segment_part_t {segment_t {wire_id, segment_index}, part(0, 5)},
        .segment_part_source =
            segment_part_t {segment_t {wire_id, segment_index}, part(5, 10)},
    }};
    ASSERT_EQ(setup.recorder.messages().size(), 3);
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
    ASSERT_EQ(setup.recorder.messages().at(1), m1);
    ASSERT_EQ(setup.recorder.messages().at(2), m2);
}

TEST(EditableCircuitHandlerWire, MoveOrDeleteWireMovePartialEnd) {
    using namespace editable_circuit::info_message;
    auto layout = Layout {};

    const auto line = ordered_line_t {point_t {0, 0}, point_t {10, 0}};
    const auto line_0 = ordered_line_t {point_t {0, 0}, point_t {5, 0}};
    const auto line_1 = ordered_line_t {point_t {105, 200}, point_t {110, 200}};

    const auto wire_id = wire_id_t {0};
    auto &m_tree = layout.wires().modifiable_segment_tree(wire_id);
    const auto segment_index = m_tree.add_segment(segment_info_t {.line = line});

    const auto segment_part_0 =
        segment_part_t {segment_t {wire_id, segment_index}, part(5, 10)};
    const auto segment_part_1 =
        segment_part_t {segment_t {wire_id, segment_index_t {1}}, part(0, 5)};

    auto setup = SenderSetup {};

    auto segment_part = segment_part_0;
    move_or_delete_wire(layout, setup.sender, segment_part, 100, 200);

    ASSERT_EQ(segment_part, segment_part_1);
    ASSERT_EQ(layout.wires().size(), 1);
    ASSERT_EQ(is_temporary(wire_id_t {0}), true);

    const auto &tree = layout.wires().segment_tree(wire_id_t {0});
    ASSERT_EQ(tree.size(), 2);
    ASSERT_EQ(tree.line(segment_index_t {0}), line_0);
    ASSERT_EQ(tree.line(segment_index_t {1}), line_1);

    // messages
    const auto m0 = Message {SegmentCreated {segment_part_1.segment}};
    const auto m1 = Message {SegmentPartMoved {
        .segment_part_destination = segment_part_1,
        .segment_part_source = segment_part_0,
    }};
    ASSERT_EQ(setup.recorder.messages().size(), 2);
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
    ASSERT_EQ(setup.recorder.messages().at(1), m1);
}

TEST(EditableCircuitHandlerWire, MoveOrDeleteWireMovePartialMiddle) {
    using namespace editable_circuit::info_message;
    auto layout = Layout {};

    const auto line = ordered_line_t {point_t {0, 0}, point_t {20, 0}};
    const auto line_0 = ordered_line_t {point_t {0, 0}, point_t {10, 0}};
    const auto line_1 = ordered_line_t {point_t {15, 0}, point_t {20, 0}};
    const auto line_2 = ordered_line_t {point_t {110, 200}, point_t {115, 200}};

    const auto wire_id = wire_id_t {0};
    auto &m_tree = layout.wires().modifiable_segment_tree(wire_id);
    const auto segment_index = m_tree.add_segment(segment_info_t {.line = line});

    const auto segment_part_0 =
        segment_part_t {segment_t {wire_id, segment_index}, part(10, 15)};

    const auto segment_part_1_from =
        segment_part_t {segment_t {wire_id, segment_index_t {0}}, part(15, 20)};
    const auto segment_part_1_to =
        segment_part_t {segment_t {wire_id, segment_index_t {1}}, part(0, 5)};

    const auto segment_part_2 =
        segment_part_t {segment_t {wire_id, segment_index_t {2}}, part(0, 5)};

    auto setup = SenderSetup {};

    auto segment_part = segment_part_0;
    move_or_delete_wire(layout, setup.sender, segment_part, 100, 200);

    ASSERT_EQ(segment_part, segment_part_2);
    ASSERT_EQ(layout.wires().size(), 1);
    ASSERT_EQ(is_temporary(wire_id_t {0}), true);

    const auto &tree = layout.wires().segment_tree(wire_id_t {0});
    ASSERT_EQ(tree.size(), 3);
    ASSERT_EQ(tree.line(segment_index_t {0}), line_0);
    ASSERT_EQ(tree.line(segment_index_t {1}), line_1);
    ASSERT_EQ(tree.line(segment_index_t {2}), line_2);

    // messages
    const auto m0 = Message {SegmentCreated {segment_part_1_to.segment}};
    const auto m1 = Message {SegmentCreated {segment_part_2.segment}};
    const auto m2 = Message {SegmentPartMoved {
        .segment_part_destination = segment_part_1_to,
        .segment_part_source = segment_part_1_from,
    }};
    const auto m3 = Message {SegmentPartMoved {
        .segment_part_destination = segment_part_2,
        .segment_part_source = segment_part_0,
    }};
    ASSERT_EQ(setup.recorder.messages().size(), 4);
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
    ASSERT_EQ(setup.recorder.messages().at(1), m1);
    ASSERT_EQ(setup.recorder.messages().at(2), m2);
    ASSERT_EQ(setup.recorder.messages().at(3), m3);
}

*/

}  // namespace logicsim