#include "component/editable_circuit/editing/edit_wire.h"
#include "component/editable_circuit/modifier.h"
#include "layout_message.h"
#include "test_core/editable_circuit/modifier/test_helpers.h"
#include "vocabulary/logicitem_definition.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

namespace editable_circuit {

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

}  // namespace

//
// add_line_segment
//

TEST(EditableCircuitModifierWire, AddTempSegment) {
    using namespace info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto modifier = get_logging_modifier(layout);
    modifier.add_wire_segment(ordered_line_t {point_t {0, 0}, point_t {10, 0}},
                              InsertionMode::temporary);
    Expects(is_valid(modifier));

    // layout
    assert_wire_count(modifier, 1);
    {
        const auto id_0 = wire_id_t {0};
        const auto &tree_0 = get_segment_tree(modifier, id_0);

        ASSERT_EQ(is_temporary(id_0), true);
        ASSERT_EQ(tree_0.size(), 1);

        const auto line = ordered_line_t {point_t {0, 0}, point_t {10, 0}};
        ASSERT_EQ(tree_0.line(segment_index_t {0}), line);
    }
    {
        const auto id_1 = wire_id_t {1};
        const auto &tree_1 = get_segment_tree(modifier, id_1);

        ASSERT_EQ(is_colliding(id_1), true);
        ASSERT_EQ(tree_1.size(), 0);
    }

    // messages
    const auto m0 = Message {SegmentCreated {
        .segment = segment_t {wire_id_t {0}, segment_index_t {0}},
        .size = offset_t {10},
    }};
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 1);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(0), m0);
}

//
// Change Mode  -  Temporary => Colliding
//

TEST(EditableCircuitModifierWire, TempToColliding) {
    using namespace info_message;
    using enum display_state_t;
    auto layout = Layout {};

    add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    add_test_wire(layout, SegmentPointType::output,
                  std::array {ordered_line_t {point_t {1, 0}, point_t {3, 0}}});

    auto segment_part = segment_part_t {
        segment_t {wire_id_t {0}, segment_index_t {0}},
        part_t {offset_t {0}, offset_t {10}},
    };

    auto modifier = get_logging_modifier(layout);
    modifier.change_wire_insertion_mode(segment_part, InsertionMode::collisions);
    Expects(is_valid(modifier));

    // layout
    assert_wire_count(modifier, 3);
    {
        const auto wire_id = wire_id_t {0};
        const auto &tree = get_segment_tree(modifier, wire_id);

        ASSERT_EQ(is_temporary(wire_id), true);
        ASSERT_EQ(tree.size(), 0);
    }
    {
        const auto wire_id = wire_id_t {1};
        const auto &tree = get_segment_tree(modifier, wire_id);

        ASSERT_EQ(is_colliding(wire_id), true);
        ASSERT_EQ(tree.size(), 1);

        const auto line = ordered_line_t {point_t {0, 0}, point_t {10, 0}};
        ASSERT_EQ(tree.line(segment_index_t {0}), line);
    }
    {
        const auto wire_id = wire_id_t {2};
        const auto &tree = get_segment_tree(modifier, wire_id);

        ASSERT_EQ(is_inserted(wire_id), true);
        ASSERT_EQ(tree.size(), 1);

        const auto line = ordered_line_t {point_t {1, 0}, point_t {3, 0}};
        ASSERT_EQ(tree.line(segment_index_t {0}), line);
    }

    // messages
    const auto m0 = Message {SegmentIdUpdated {
        .new_segment = segment_t {wire_id_t {1}, segment_index_t {0}},
        .old_segment = segment_t {wire_id_t {0}, segment_index_t {0}},
    }};

    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 1);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(0), m0);
}

TEST(EditableCircuitModifierWire, TempToCollidingPartialOneSide) {
    using namespace info_message;
    using enum display_state_t;
    auto layout = Layout {};

    add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    add_test_wire(layout, SegmentPointType::output,
                  std::array {ordered_line_t {point_t {1, 0}, point_t {3, 0}}});

    auto segment_part = segment_part_t {
        segment_t {wire_id_t {0}, segment_index_t {0}},
        part_t {0, 5},
    };

    auto modifier = get_logging_modifier(layout);
    modifier.change_wire_insertion_mode(segment_part, InsertionMode::collisions);
    Expects(is_valid(modifier));

    // layout
    assert_wire_count(modifier, 3);
    {
        const auto wire_id = wire_id_t {0};
        const auto &tree = get_segment_tree(modifier, wire_id);

        ASSERT_EQ(is_temporary(wire_id), true);
        ASSERT_EQ(tree.size(), 1);

        const auto line = ordered_line_t {point_t {5, 0}, point_t {10, 0}};
        ASSERT_EQ(tree.line(segment_index_t {0}), line);
    }
    {
        const auto wire_id = wire_id_t {1};
        const auto &tree = get_segment_tree(modifier, wire_id);

        ASSERT_EQ(is_colliding(wire_id), true);
        ASSERT_EQ(tree.size(), 1);

        const auto line = ordered_line_t {point_t {0, 0}, point_t {5, 0}};
        ASSERT_EQ(tree.line(segment_index_t {0}), line);
    }
    {
        const auto wire_id = wire_id_t {2};
        const auto &tree = get_segment_tree(modifier, wire_id);

        ASSERT_EQ(is_inserted(wire_id), true);
        ASSERT_EQ(tree.size(), 1);

        const auto line = ordered_line_t {point_t {1, 0}, point_t {3, 0}};
        ASSERT_EQ(tree.line(segment_index_t {0}), line);
    }

    // messages
    const auto m0 = Message {SegmentPartMoved {
        .destination =
            segment_part_t {
                segment_t {wire_id_t {1}, segment_index_t {0}},
                part_t {0, 5},
            },
        .source =
            segment_part_t {
                segment_t {wire_id_t {0}, segment_index_t {0}},
                part_t {0, 5},
            },
    }};
    const auto m1 = Message {SegmentPartMoved {
        .destination =
            segment_part_t {
                segment_t {wire_id_t {0}, segment_index_t {0}},
                part_t {0, 5},
            },
        .source =
            segment_part_t {
                segment_t {wire_id_t {0}, segment_index_t {0}},
                part_t {5, 10},
            },
    }};
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 2);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(0), m0);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(1), m1);
}

TEST(EditableCircuitModifierWire, TempToCollidingPartialMiddle) {
    using namespace info_message;
    using enum display_state_t;
    auto layout = Layout {};

    add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    add_test_wire(layout, SegmentPointType::output,
                  std::array {ordered_line_t {point_t {1, 0}, point_t {3, 0}}});

    auto segment_part = segment_part_t {
        segment_t {wire_id_t {0}, segment_index_t {0}},
        part_t {offset_t {2}, offset_t {5}},
    };

    auto modifier = get_logging_modifier(layout);
    modifier.change_wire_insertion_mode(segment_part, InsertionMode::collisions);
    Expects(is_valid(modifier));

    // layout
    assert_wire_count(modifier, 3);
    {
        const auto wire_id = wire_id_t {0};
        const auto &tree = get_segment_tree(modifier, wire_id);

        ASSERT_EQ(is_temporary(wire_id), true);
        ASSERT_EQ(tree.size(), 2);

        const auto line0 = ordered_line_t {point_t {0, 0}, point_t {2, 0}};
        const auto line1 = ordered_line_t {point_t {5, 0}, point_t {10, 0}};
        ASSERT_EQ(tree.line(segment_index_t {0}), line0);
        ASSERT_EQ(tree.line(segment_index_t {1}), line1);
    }
    {
        const auto wire_id = wire_id_t {1};
        const auto &tree = get_segment_tree(modifier, wire_id);

        ASSERT_EQ(is_colliding(wire_id), true);
        ASSERT_EQ(tree.size(), 1);

        const auto line = ordered_line_t {point_t {2, 0}, point_t {5, 0}};
        ASSERT_EQ(tree.line(segment_index_t {0}), line);
    }
    {
        const auto wire_id = wire_id_t {2};
        const auto &tree = get_segment_tree(modifier, wire_id);

        ASSERT_EQ(is_inserted(wire_id), true);
        ASSERT_EQ(tree.size(), 1);

        const auto line = ordered_line_t {point_t {1, 0}, point_t {3, 0}};
        ASSERT_EQ(tree.line(segment_index_t {0}), line);
    }

    // messages
    const auto m0 = Message {SegmentPartMoved {
        .destination =
            segment_part_t {
                segment_t {wire_id_t {0}, segment_index_t {1}},
                part_t {offset_t {0}, offset_t {5}},
            },
        .source =
            segment_part_t {
                segment_t {wire_id_t {0}, segment_index_t {0}},
                part_t {offset_t {5}, offset_t {10}},
            },
    }};
    const auto m1 = Message {SegmentPartMoved {
        .destination =
            segment_part_t {
                segment_t {wire_id_t {1}, segment_index_t {0}},
                part_t {offset_t {0}, offset_t {3}},
            },
        .source =
            segment_part_t {
                segment_t {wire_id_t {0}, segment_index_t {0}},
                part_t {offset_t {2}, offset_t {5}},
            },
    }};

    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 2);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(0), m0);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(1), m1);
}

//
// Change Mode  -  Temporary => Valid
//

TEST(EditableCircuitModifierWire, TempToValid) {
    using namespace info_message;
    using enum display_state_t;
    auto layout = Layout {};

    add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});

    auto segment_part = segment_part_t {
        segment_t {wire_id_t {0}, segment_index_t {0}},
        part_t {offset_t {0}, offset_t {10}},
    };

    const auto info_0 = get_segment_info(layout, segment_part.segment);

    auto modifier = get_logging_modifier(layout);
    modifier.change_wire_insertion_mode(segment_part, InsertionMode::collisions);
    Expects(is_valid(modifier));

    // layout
    assert_wire_count(modifier, 3);
    {
        const auto wire_id = wire_id_t {0};
        const auto &tree = get_segment_tree(modifier, wire_id);

        ASSERT_EQ(is_temporary(wire_id), true);
        ASSERT_EQ(tree.size(), 0);
    }
    {
        const auto wire_id = wire_id_t {1};
        const auto &tree = get_segment_tree(modifier, wire_id);

        ASSERT_EQ(is_colliding(wire_id), true);
        ASSERT_EQ(tree.size(), 0);
    }
    {
        const auto wire_id = wire_id_t {2};
        const auto &tree = get_segment_tree(modifier, wire_id);

        ASSERT_EQ(is_inserted(wire_id), true);
        ASSERT_EQ(tree.size(), 1);

        const auto line = ordered_line_t {point_t {0, 0}, point_t {10, 0}};
        ASSERT_EQ(tree.line(segment_index_t {0}), line);

        ASSERT_EQ(tree.valid_parts(segment_index_t {0}).size(), 1);
        ASSERT_EQ(tree.valid_parts(segment_index_t {0}).front(), part_t(0, 10));
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

    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 4);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(0), m0);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(1), m1);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(2), m2);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(3), m3);
}

//
// is_wire_position_representable
//

TEST(EditableCircuitModifierWire, IsWirePositionRepresentable) {
    using namespace editing;
    auto layout = Layout {};

    const auto wire_id = wire_id_t {0};
    auto &m_tree = layout.wires().modifiable_segment_tree(wire_id);
    const auto segment_index = m_tree.add_segment(
        segment_info_t {.line = ordered_line_t {point_t {0, 0}, point_t {10, 0}}});

    const auto segment = segment_t {wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {0, 10}};

    constexpr static auto overflow = int {grid_t::max()} + 100;

    ASSERT_EQ(is_wire_position_representable(layout, segment_part, 10, 10), true);
    ASSERT_EQ(is_wire_position_representable(layout, segment_part, -10, -10), true);

    ASSERT_EQ(is_wire_position_representable(layout, segment_part, overflow, 10), false);
    ASSERT_EQ(is_wire_position_representable(layout, segment_part, -overflow, 10), false);
    ASSERT_EQ(is_wire_position_representable(layout, segment_part, 0, overflow), false);
}

TEST(EditableCircuitModifierWire, IsWirePositionRepresentablePart) {
    using namespace editing;
    auto layout = Layout {};

    auto p1_x = grid_t::max();

    const auto wire_id = wire_id_t {0};
    auto &m_tree = layout.wires().modifiable_segment_tree(wire_id);
    const auto segment_index = m_tree.add_segment(
        segment_info_t {.line = ordered_line_t {point_t {0, 0}, point_t {p1_x, 0}}});

    const auto segment = segment_t {wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {0, 10}};
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

TEST(EditableCircuitModifierWire, MoveOrDeleteWireMove) {
    using namespace info_message;
    auto layout = Layout {};

    const auto line = ordered_line_t {point_t {0, 0}, point_t {10, 0}};
    const auto line_0 = ordered_line_t {point_t {100, 200}, point_t {110, 200}};

    const auto wire_id = wire_id_t {0};
    auto &m_tree = layout.wires().modifiable_segment_tree(wire_id);
    const auto segment_index = m_tree.add_segment(segment_info_t {.line = line});

    const auto segment_part_0 = segment_part_t {
        segment_t {wire_id, segment_index},
        part_t {0, 10},
    };

    auto modifier = get_logging_modifier(layout);

    auto segment_part = segment_part_0;
    modifier.move_or_delete_temporary_wire(segment_part, 100, 200);
    Expects(is_valid(modifier));

    assert_wire_count(modifier, 1);
    ASSERT_EQ(is_temporary(wire_id_t {0}), true);

    ASSERT_EQ(segment_part, segment_part_0);

    const auto &tree = get_segment_tree(modifier, wire_id_t {0});
    ASSERT_EQ(tree.size(), 1);
    ASSERT_EQ(tree.line(segment_index_t {0}), line_0);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 0);
}

TEST(EditableCircuitModifierWire, MoveOrDeleteWireMovePartialBegin) {
    using namespace info_message;
    auto layout = Layout {};

    const auto line = ordered_line_t {point_t {0, 0}, point_t {10, 0}};
    const auto line_0 = ordered_line_t {point_t {5, 0}, point_t {10, 0}};
    const auto line_1 = ordered_line_t {point_t {100, 200}, point_t {105, 200}};

    const auto wire_id = wire_id_t {0};
    auto &m_tree = layout.wires().modifiable_segment_tree(wire_id);
    const auto segment_index = m_tree.add_segment(segment_info_t {.line = line});

    const auto segment_part_0 =
        segment_part_t {segment_t {wire_id, segment_index}, part_t {0, 5}};
    const auto segment_part_1 =
        segment_part_t {segment_t {wire_id, segment_index_t {1}}, part_t {0, 5}};

    auto modifier = get_logging_modifier(layout);

    auto segment_part = segment_part_0;
    modifier.move_or_delete_temporary_wire(segment_part, 100, 200);
    Expects(is_valid(modifier));

    ASSERT_EQ(segment_part, segment_part_1);
    assert_wire_count(modifier, 1);
    ASSERT_EQ(is_temporary(wire_id_t {0}), true);

    const auto &tree = get_segment_tree(modifier, wire_id_t {0});
    ASSERT_EQ(tree.size(), 2);
    ASSERT_EQ(tree.line(segment_index_t {0}), line_0);
    ASSERT_EQ(tree.line(segment_index_t {1}), line_1);

    // messages
    const auto m0 = Message {SegmentPartMoved {
        .destination = segment_part_1,
        .source = segment_part_0,
    }};
    const auto m1 = Message {SegmentPartMoved {
        .destination = segment_part_t {segment_t {wire_id, segment_index}, part_t {0, 5}},
        .source = segment_part_t {segment_t {wire_id, segment_index}, part_t {5, 10}},
    }};
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 2);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(0), m0);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(1), m1);
}

TEST(EditableCircuitModifierWire, MoveOrDeleteWireMovePartialEnd) {
    using namespace info_message;
    auto layout = Layout {};

    const auto line = ordered_line_t {point_t {0, 0}, point_t {10, 0}};
    const auto line_0 = ordered_line_t {point_t {0, 0}, point_t {5, 0}};
    const auto line_1 = ordered_line_t {point_t {105, 200}, point_t {110, 200}};

    const auto wire_id = wire_id_t {0};
    auto &m_tree = layout.wires().modifiable_segment_tree(wire_id);
    const auto segment_index = m_tree.add_segment(segment_info_t {.line = line});

    const auto segment_part_0 =
        segment_part_t {segment_t {wire_id, segment_index}, part_t {5, 10}};
    const auto segment_part_1 =
        segment_part_t {segment_t {wire_id, segment_index_t {1}}, part_t {0, 5}};

    auto modifier = get_logging_modifier(layout);

    auto segment_part = segment_part_0;
    modifier.move_or_delete_temporary_wire(segment_part, 100, 200);
    Expects(is_valid(modifier));

    ASSERT_EQ(segment_part, segment_part_1);
    assert_wire_count(modifier, 1);
    ASSERT_EQ(is_temporary(wire_id_t {0}), true);

    const auto &tree = get_segment_tree(modifier, wire_id_t {0});
    ASSERT_EQ(tree.size(), 2);
    ASSERT_EQ(tree.line(segment_index_t {0}), line_0);
    ASSERT_EQ(tree.line(segment_index_t {1}), line_1);

    // messages
    const auto m0 = Message {SegmentPartMoved {
        .destination = segment_part_1,
        .source = segment_part_0,
    }};
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 1);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(0), m0);
}

TEST(EditableCircuitModifierWire, MoveOrDeleteWireMovePartialMiddle) {
    using namespace info_message;
    auto layout = Layout {};

    const auto line = ordered_line_t {point_t {0, 0}, point_t {20, 0}};
    const auto line_0 = ordered_line_t {point_t {0, 0}, point_t {10, 0}};
    const auto line_1 = ordered_line_t {point_t {15, 0}, point_t {20, 0}};
    const auto line_2 = ordered_line_t {point_t {110, 200}, point_t {115, 200}};

    const auto wire_id = wire_id_t {0};
    auto &m_tree = layout.wires().modifiable_segment_tree(wire_id);
    const auto segment_index = m_tree.add_segment(segment_info_t {.line = line});

    const auto segment_part_0 = segment_part_t {
        segment_t {wire_id, segment_index},
        part_t {10, 15},
    };

    const auto segment_part_1_from = segment_part_t {
        segment_t {wire_id, segment_index_t {0}},
        part_t {15, 20},
    };
    const auto segment_part_1_to = segment_part_t {
        segment_t {wire_id, segment_index_t {1}},
        part_t {0, 5},
    };

    const auto segment_part_2 = segment_part_t {
        segment_t {wire_id, segment_index_t {2}},
        part_t {0, 5},
    };

    auto segment_part = segment_part_0;

    auto modifier = get_logging_modifier(layout);
    modifier.move_or_delete_temporary_wire(segment_part, 100, 200);
    Expects(is_valid(modifier));

    ASSERT_EQ(segment_part, segment_part_2);
    assert_wire_count(modifier, 1);
    ASSERT_EQ(is_temporary(wire_id_t {0}), true);

    const auto &tree = get_segment_tree(modifier, wire_id_t {0});
    ASSERT_EQ(tree.size(), 3);
    ASSERT_EQ(tree.line(segment_index_t {0}), line_0);
    ASSERT_EQ(tree.line(segment_index_t {1}), line_1);
    ASSERT_EQ(tree.line(segment_index_t {2}), line_2);

    // messages
    const auto m0 = Message {SegmentPartMoved {
        .destination = segment_part_1_to,
        .source = segment_part_1_from,
    }};
    const auto m1 = Message {SegmentPartMoved {
        .destination = segment_part_2,
        .source = segment_part_0,
    }};
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 2);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(0), m0);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(1), m1);
}

}  // namespace editable_circuit

}  // namespace logicsim