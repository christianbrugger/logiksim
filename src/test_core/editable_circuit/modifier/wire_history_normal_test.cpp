
#include "test_core/editable_circuit/modifier/test_helpers.h"

#include "core/component/editable_circuit/editing/edit_wire.h"
#include "core/component/editable_circuit/key_state.h"
#include "core/component/editable_circuit/modifier.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

namespace editable_circuit {

//
// Delete
//

TEST(EditableCircuitWireHistory, DeleteFullShadow) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {0, 10}};

    auto modifier = get_modifier_with_history(layout);
    const auto state_0 = layout_key_state_t {modifier};
    {
        auto segment_part_0 = segment_part;
        modifier.delete_temporary_wire_segment(segment_part_0);
    }
    const auto state_1 = layout_key_state_t {modifier};
    Expects(is_valid(modifier));

    // before undo
    ASSERT_TRUE(get_segment_count(modifier.circuit_data().layout) == 0);
    ASSERT_TRUE(state_0 != state_1);

    // after undo
    modifier.undo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_0);

    // after redo
    modifier.redo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_1);
}

TEST(EditableCircuitWireHistory, DeleteFullCrosspoint) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::cross_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {0, 10}};

    auto modifier = get_modifier_with_history(layout);
    const auto state_0 = layout_key_state_t {modifier};
    {
        auto segment_part_0 = segment_part;
        modifier.delete_temporary_wire_segment(segment_part_0);
    }
    const auto state_1 = layout_key_state_t {modifier};
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(get_segment_count(modifier.circuit_data().layout), 0);
    ASSERT_TRUE(state_0 != state_1);

    // after undo
    modifier.undo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_0);

    // after redo
    modifier.redo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_1);
}

TEST(EditableCircuitWireHistory, DeletePartialFront) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {0, 5}};

    auto modifier = get_modifier_with_history(layout);
    const auto state_0 = layout_key_state_t {modifier};
    {
        auto segment_part_0 = segment_part;
        modifier.delete_temporary_wire_segment(segment_part_0);
    }
    const auto state_1 = layout_key_state_t {modifier};
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(get_segment_count(modifier.circuit_data().layout), 1);
    ASSERT_TRUE(state_0 != state_1);

    // after undo
    modifier.undo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_0);

    // after redo
    modifier.redo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_1);
}

TEST(EditableCircuitWireHistory, DeletePartialEnd) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {5, 10}};

    auto modifier = get_modifier_with_history(layout);
    const auto state_0 = layout_key_state_t {modifier};
    {
        auto segment_part_0 = segment_part;
        modifier.delete_temporary_wire_segment(segment_part_0);
    }
    const auto state_1 = layout_key_state_t {modifier};
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(get_segment_count(modifier.circuit_data().layout), 1);
    ASSERT_TRUE(state_0 != state_1);

    // after undo
    modifier.undo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_0);

    // after redo
    modifier.redo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_1);
}

TEST(EditableCircuitWireHistory, DeletePartialMiddle) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {3, 6}};

    auto modifier = get_modifier_with_history(layout);
    const auto state_0 = layout_key_state_t {modifier};
    {
        auto segment_part_0 = segment_part;
        modifier.delete_temporary_wire_segment(segment_part_0);
    }
    const auto state_1 = layout_key_state_t {modifier};
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(get_segment_count(modifier.circuit_data().layout), 2);
    ASSERT_TRUE(state_0 != state_1);

    // after undo
    modifier.undo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_0);

    // after redo
    modifier.redo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_1);
}

//
// Move
//

TEST(EditableCircuitWireHistory, MoveFull) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {0, 10}};

    auto modifier = get_modifier_with_history(layout);
    const auto state_0 = layout_key_state_t {modifier};
    {
        auto segment_part_0 = segment_part;
        modifier.move_or_delete_temporary_wire(segment_part_0, move_delta_t {10, 10});
    }
    const auto state_1 = layout_key_state_t {modifier};
    Expects(is_valid(modifier));

    // before undo
    ASSERT_TRUE(state_0 != state_1);
    const auto moved_line = ordered_line_t {point_t {10, 10}, point_t {20, 10}};
    ASSERT_TRUE(get_line(modifier.circuit_data().layout, segment) == moved_line);

    // after undo
    modifier.undo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_0);

    // after redo
    modifier.redo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_1);
}

TEST(EditableCircuitWireHistory, MovePartialMiddle) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {2, 7}};

    auto modifier = get_modifier_with_history(layout);
    const auto state_0 = layout_key_state_t {modifier};
    {
        auto segment_part_0 = segment_part;
        modifier.move_or_delete_temporary_wire(segment_part_0, move_delta_t {10, 10});
    }
    const auto state_1 = layout_key_state_t {modifier};
    Expects(is_valid(modifier));

    // before undo
    ASSERT_TRUE(state_0 != state_1);
    ASSERT_TRUE(get_segment_count(modifier.circuit_data().layout) == 3);

    // after undo
    modifier.undo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_0);

    // after redo
    modifier.redo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_1);
}

TEST(EditableCircuitWireHistory, MovePartialDelete) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {2, 7}};

    auto modifier = get_modifier_with_history(layout);
    const auto state_0 = layout_key_state_t {modifier};
    {
        auto segment_part_0 = segment_part;
        const auto overflow = int {offset_t::max()};
        modifier.move_or_delete_temporary_wire(segment_part_0,
                                               move_delta_t {overflow, overflow});
    }
    const auto state_1 = layout_key_state_t {modifier};
    Expects(is_valid(modifier));

    // before undo
    ASSERT_TRUE(get_segment_count(modifier.circuit_data().layout) == 2);
    ASSERT_TRUE(state_0 != state_1);

    // after undo
    modifier.undo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_0);

    // after redo
    modifier.redo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_1);
}

//
// Create
//

TEST(EditableCircuitWireHistory, AddTemporary) {
    auto layout = Layout {};

    auto modifier = get_modifier_with_history(layout);
    const auto state_0 = layout_key_state_t {modifier};
    modifier.add_wire_segment(ordered_line_t {point_t {0, 0}, point_t {10, 0}},
                              InsertionMode::temporary);
    const auto state_1 = layout_key_state_t {modifier};
    Expects(is_valid(modifier));

    // before undo
    ASSERT_TRUE(get_segment_count(modifier.circuit_data().layout) == 1);
    ASSERT_TRUE(state_0 != state_1);

    // after undo
    modifier.undo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_0);

    // after redo
    modifier.redo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_1);
}

}  // namespace editable_circuit

}  // namespace logicsim
