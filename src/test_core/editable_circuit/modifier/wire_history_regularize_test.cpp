
#include "test_core/editable_circuit/modifier/test_helpers.h"

#include "core/component/editable_circuit/editing/edit_wire.h"
#include "core/component/editable_circuit/key_state.h"
#include "core/component/editable_circuit/modifier.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

namespace editable_circuit {

//
// Set Temporary Endpoints
//

TEST(EditableCircuitWireHistory, SetEndpointsCross) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};

    auto modifier = get_modifier_with_history(layout);
    const auto state_0 = layout_key_state_t {modifier};
    {
        modifier.set_temporary_endpoints(
            segment,
            endpoints_t {SegmentPointType::cross_point, SegmentPointType::shadow_point});
    }
    const auto state_1 = layout_key_state_t {modifier};
    Expects(is_valid(modifier));

    // before undo
    ASSERT_TRUE(state_0 != state_1);

    // after undo
    modifier.undo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_0);

    // after redo
    modifier.redo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_1);
}

//
// Merge Uninserted Segment (Single)
//

namespace {

auto _test_merge_single_restore(ordered_line_t line_0, ordered_line_t line_1,
                                bool flip_merge) {
    // setup
    auto layout = Layout {};
    const auto segment_index_0 =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point, line_0);
    const auto segment_index_1 =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point, line_1);
    const auto segment_0 = segment_t {temporary_wire_id, segment_index_0};
    const auto segment_1 = segment_t {temporary_wire_id, segment_index_1};

    // build history
    auto modifier = get_modifier_with_history(layout);
    const auto state_0 = layout_key_state_t {modifier};
    const auto segment_key_0 = modifier.circuit_data().index.key_index().get(segment_0);
    const auto segment_key_1 = modifier.circuit_data().index.key_index().get(segment_1);
    const auto segment_merged =
        flip_merge ? modifier.merge_uninserted_segment(segment_1, segment_0)
                   : modifier.merge_uninserted_segment(segment_0, segment_1);
    const auto state_1 = layout_key_state_t {modifier};
    Expects(is_valid(modifier));

    // before undo
    ASSERT_TRUE(state_0 != state_1);
    const auto expected_merged_key = line_0 < line_1 ? segment_key_0 : segment_key_1;
    ASSERT_EQ(modifier.circuit_data().index.key_index().get(segment_merged),
              expected_merged_key);

    // after undo
    modifier.undo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_0);

    // after redo
    modifier.redo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_1);
}

}  // namespace

TEST(EditableCircuitWireHistory, MergeSingleRestoreOrdered) {
    const auto line_0 = ordered_line_t {point_t {0, 0}, point_t {5, 0}};
    const auto line_1 = ordered_line_t {point_t {5, 0}, point_t {10, 0}};
    const auto flip_merge = false;
    _test_merge_single_restore(line_0, line_1, flip_merge);
}

TEST(EditableCircuitWireHistory, MergeSingleRestoreFlipped1) {
    const auto line_0 = ordered_line_t {point_t {0, 0}, point_t {5, 0}};
    const auto line_1 = ordered_line_t {point_t {5, 0}, point_t {10, 0}};
    const auto flip_merge = true;
    _test_merge_single_restore(line_0, line_1, flip_merge);
}

TEST(EditableCircuitWireHistory, MergeSingleRestoreFlipped2) {
    const auto line_0 = ordered_line_t {point_t {5, 0}, point_t {10, 0}};
    const auto line_1 = ordered_line_t {point_t {0, 0}, point_t {5, 0}};
    const auto flip_merge = false;
    _test_merge_single_restore(line_0, line_1, flip_merge);
}

TEST(EditableCircuitWireHistory, MergeSingleRestoreFlipped3) {
    const auto line_0 = ordered_line_t {point_t {5, 0}, point_t {10, 0}};
    const auto line_1 = ordered_line_t {point_t {0, 0}, point_t {5, 0}};
    const auto flip_merge = true;
    _test_merge_single_restore(line_0, line_1, flip_merge);
}

//
// Split Uninserted Segment (Single)
//

TEST(EditableCircuitWireHistory, SplitTemporary) {
    auto layout = Layout {};
    const auto line_orig = ordered_line_t {point_t {0, 0}, point_t {10, 0}};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point, line_orig);
    const auto segment = segment_t {temporary_wire_id, segment_index};

    auto modifier = get_modifier_with_history(layout);
    const auto state_0 = layout_key_state_t {modifier};
    const auto segment_key = modifier.circuit_data().index.key_index().get(segment);
    const auto new_key = [=]() {
        auto key = segment_key;
        return ++(++(++key));
    }();
    modifier.split_uninserted_segment(segment, offset_t {5}, new_key);
    const auto state_1 = layout_key_state_t {modifier};
    Expects(is_valid(modifier));

    // before undo
    ASSERT_TRUE(state_0 != state_1);
    const auto line_0 = ordered_line_t {point_t {0, 0}, point_t {5, 0}};
    const auto line_1 = ordered_line_t {point_t {5, 0}, point_t {10, 0}};
    ASSERT_EQ(get_line(modifier.circuit_data().layout,
                       modifier.circuit_data().index.key_index().get(segment_key)),
              line_0);
    ASSERT_EQ(get_line(modifier.circuit_data().layout,
                       modifier.circuit_data().index.key_index().get(new_key)),
              line_1);

    // after undo
    modifier.undo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_0);

    // after redo
    modifier.redo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_1);
}

//
// Split Temporary Segments (Multiple)
//

TEST(EditableCircuitWireHistory, SplitsTemporarySingle) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {0, 10}};

    auto modifier = get_modifier_with_history(layout);
    const auto state_0 = layout_key_state_t {modifier};
    {
        auto selection = Selection {};
        selection.add_segment(segment_part);
        modifier.split_temporary_segments(selection, std::array {point_t {5, 0}});
    }
    const auto state_1 = layout_key_state_t {modifier};
    Expects(is_valid(modifier));

    // before undo
    ASSERT_TRUE(state_0 != state_1);

    // after undo
    modifier.undo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_0);

    // after redo
    modifier.redo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_1);
}

TEST(EditableCircuitWireHistory, SplitsTemporaryMultiple) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {0, 10}};

    auto modifier = get_modifier_with_history(layout);
    const auto state_0 = layout_key_state_t {modifier};
    {
        auto selection = Selection {};
        selection.add_segment(segment_part);
        modifier.split_temporary_segments(
            selection, std::array {point_t {5, 0}, point_t {4, 0}, point_t {3, 0},
                                   point_t {7, 0}, point_t {8, 0}, point_t {9, 0}});
    }
    const auto state_1 = layout_key_state_t {modifier};
    Expects(is_valid(modifier));

    // before undo
    ASSERT_TRUE(state_0 != state_1);

    // after undo
    modifier.undo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_0);

    // after redo
    modifier.redo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_1);
}

//
// Regularize Temporary Selection
//

TEST(EditableCircuitWireHistory, RegularizeMergeSingle) {
    // setup
    auto layout = Layout {};
    const auto line_0 = ordered_line_t {point_t {0, 0}, point_t {5, 0}};
    const auto line_1 = ordered_line_t {point_t {5, 0}, point_t {10, 0}};
    const auto segment_index_0 =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point, line_0);
    const auto segment_index_1 =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point, line_1);
    const auto segment_part_0 =
        segment_part_t {segment_t {temporary_wire_id, segment_index_0}, part_t {0, 5}};
    const auto segment_part_1 =
        segment_part_t {segment_t {temporary_wire_id, segment_index_1}, part_t {0, 5}};

    // build history
    auto modifier = get_modifier_with_history(layout);
    const auto state_0 = layout_key_state_t {modifier};
    {
        auto selection = Selection {};
        selection.add_segment(segment_part_0);
        selection.add_segment(segment_part_1);
        modifier.regularize_temporary_selection(selection, {});
    }
    const auto state_1 = layout_key_state_t {modifier};
    Expects(is_valid(modifier));

    // before undo
    ASSERT_TRUE(state_0 != state_1);

    // after undo
    modifier.undo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_0);

    // after redo
    modifier.redo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_1);
}

TEST(EditableCircuitWireHistory, RegularizeSetCrosspoint) {
    // setup
    auto layout = Layout {};
    const auto line_0 = ordered_line_t {point_t {0, 0}, point_t {5, 0}};
    const auto line_1 = ordered_line_t {point_t {5, 0}, point_t {10, 0}};
    const auto line_2 = ordered_line_t {point_t {5, 0}, point_t {5, 10}};
    const auto segment_index_0 =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point, line_0);
    const auto segment_index_1 =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point, line_1);
    const auto segment_index_2 =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point, line_2);
    const auto segment_part_0 =
        segment_part_t {segment_t {temporary_wire_id, segment_index_0}, part_t {0, 5}};
    const auto segment_part_1 =
        segment_part_t {segment_t {temporary_wire_id, segment_index_1}, part_t {0, 5}};
    const auto segment_part_2 =
        segment_part_t {segment_t {temporary_wire_id, segment_index_2}, part_t {0, 10}};

    // build history
    auto modifier = get_modifier_with_history(layout);
    const auto state_0 = layout_key_state_t {modifier};
    {
        auto selection = Selection {};
        selection.add_segment(segment_part_0);
        selection.add_segment(segment_part_1);
        selection.add_segment(segment_part_2);
        modifier.regularize_temporary_selection(selection, {});
    }
    const auto state_1 = layout_key_state_t {modifier};
    Expects(is_valid(modifier));

    // before undo
    ASSERT_TRUE(state_0 != state_1);

    // after undo
    modifier.undo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_0);

    // after redo
    modifier.redo_group();
    ASSERT_TRUE(layout_key_state_t {modifier} == state_1);
}

TEST(EditableCircuitWireHistory, RegularizeTrueCrosspoint) {
    // setup
    auto layout = Layout {};
    const auto line_0 = ordered_line_t {point_t {0, 0}, point_t {10, 0}};
    const auto line_1 = ordered_line_t {point_t {5, -5}, point_t {5, 5}};
    const auto segment_index_0 =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point, line_0);
    const auto segment_index_1 =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point, line_1);
    const auto segment_part_0 =
        segment_part_t {segment_t {temporary_wire_id, segment_index_0}, part_t {0, 10}};
    const auto segment_part_1 =
        segment_part_t {segment_t {temporary_wire_id, segment_index_1}, part_t {0, 10}};

    // build history
    auto modifier = get_modifier_with_history(layout);
    const auto state_0 = layout_key_state_t {modifier};
    {
        auto guard = ModifierSelectionGuard {modifier};
        modifier.add_to_selection(guard.selection_id(), segment_part_0);
        modifier.add_to_selection(guard.selection_id(), segment_part_1);
        const auto& selection =
            modifier.circuit_data().selection_store.at(guard.selection_id());
        modifier.regularize_temporary_selection(selection, std::vector {point_t {5, 0}});
    }
    const auto state_1 = layout_key_state_t {modifier};
    Expects(is_valid(modifier));

    // before undo
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
