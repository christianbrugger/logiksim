
#include "test_core/editable_circuit/modifier/test_helpers.h"

#include "core/component/editable_circuit/editing/edit_wire.h"
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
    {
        modifier.set_temporary_endpoints(
            segment,
            endpoints_t {SegmentPointType::cross_point, SegmentPointType::shadow_point});
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
}

//
// Merge Uninserted Segment (Single)
//

TEST(EditableCircuitWireHistory, MergeSingleRestoreOrdered) {
    // setup
    auto layout = Layout {};
    const auto line_0 = ordered_line_t {point_t {0, 0}, point_t {5, 0}};
    const auto line_1 = ordered_line_t {point_t {5, 0}, point_t {10, 0}};
    const auto segment_index_0 =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point, line_0);
    const auto segment_index_1 =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point, line_1);
    const auto segment_0 = segment_t {temporary_wire_id, segment_index_0};
    const auto segment_1 = segment_t {temporary_wire_id, segment_index_1};

    // build history
    auto modifier = get_modifier_with_history(layout);
    const auto segment_key_0 = modifier.circuit_data().index.key_index().get(segment_0);
    const auto segment_key_1 = modifier.circuit_data().index.key_index().get(segment_1);
    const auto segment_merged = modifier.merge_uninserted_segment(segment_0, segment_1);
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);
    ASSERT_EQ(modifier.circuit_data().index.key_index().get(segment_merged),
              segment_key_0);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);

    ASSERT_EQ(get_line(modifier.circuit_data().layout,
                       modifier.circuit_data().index.key_index().get(segment_key_0)),
              line_0);
    ASSERT_EQ(get_line(modifier.circuit_data().layout,
                       modifier.circuit_data().index.key_index().get(segment_key_1)),
              line_1);
}

TEST(EditableCircuitWireHistory, MergeSingleRestoreFlipped) {
    // setup
    auto layout = Layout {};
    const auto line_0 = ordered_line_t {point_t {0, 0}, point_t {5, 0}};
    const auto line_1 = ordered_line_t {point_t {5, 0}, point_t {10, 0}};
    const auto segment_index_0 =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point, line_0);
    const auto segment_index_1 =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point, line_1);
    const auto segment_0 = segment_t {temporary_wire_id, segment_index_0};
    const auto segment_1 = segment_t {temporary_wire_id, segment_index_1};

    // build history
    auto modifier = get_modifier_with_history(layout);
    const auto segment_key_0 = modifier.circuit_data().index.key_index().get(segment_0);
    const auto segment_key_1 = modifier.circuit_data().index.key_index().get(segment_1);
    const auto segment_merged = modifier.merge_uninserted_segment(segment_1, segment_0);
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);
    ASSERT_EQ(modifier.circuit_data().index.key_index().get(segment_merged),
              segment_key_0);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);

    ASSERT_EQ(get_line(modifier.circuit_data().layout,
                       modifier.circuit_data().index.key_index().get(segment_key_0)),
              line_0);
    ASSERT_EQ(get_line(modifier.circuit_data().layout,
                       modifier.circuit_data().index.key_index().get(segment_key_1)),
              line_1);
}

//
// Split Uninserted Segment (Single)
//

//
// Split Temporary Segments (Multiple)
//

TEST(EditableCircuitWireHistory, SplitTemporarySingle) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {0, 10}};

    auto modifier = get_modifier_with_history(layout);
    const auto segment_key = modifier.circuit_data().index.key_index().get(segment);
    {
        auto selection = Selection {};
        selection.add_segment(segment_part);
        modifier.split_temporary_segments(selection, std::array {point_t {5, 0}});
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key, modifier.circuit_data().index.key_index().get(segment));
}

TEST(EditableCircuitWireHistory, SplitTemporaryMultiple) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {0, 10}};

    auto modifier = get_modifier_with_history(layout);
    const auto segment_key = modifier.circuit_data().index.key_index().get(segment);
    {
        auto selection = Selection {};
        selection.add_segment(segment_part);
        modifier.split_temporary_segments(
            selection, std::array {point_t {5, 0}, point_t {4, 0}, point_t {3, 0},
                                   point_t {7, 0}, point_t {8, 0}, point_t {9, 0}});
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key, modifier.circuit_data().index.key_index().get(segment));
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
    const auto segment_key_0 =
        modifier.circuit_data().index.key_index().get(segment_part_0.segment);
    const auto segment_key_1 =
        modifier.circuit_data().index.key_index().get(segment_part_1.segment);
    {
        auto selection = Selection {};
        selection.add_segment(segment_part_0);
        selection.add_segment(segment_part_1);
        modifier.regularize_temporary_selection(selection, {});
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);

    ASSERT_EQ(get_line(modifier.circuit_data().layout,
                       modifier.circuit_data().index.key_index().get(segment_key_0)),
              line_0);
    ASSERT_EQ(get_line(modifier.circuit_data().layout,
                       modifier.circuit_data().index.key_index().get(segment_key_1)),
              line_1);
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
    const auto segment_key_0 =
        modifier.circuit_data().index.key_index().get(segment_part_0.segment);
    const auto segment_key_1 =
        modifier.circuit_data().index.key_index().get(segment_part_1.segment);
    const auto segment_key_2 =
        modifier.circuit_data().index.key_index().get(segment_part_2.segment);
    {
        auto selection = Selection {};
        selection.add_segment(segment_part_0);
        selection.add_segment(segment_part_1);
        selection.add_segment(segment_part_2);
        modifier.regularize_temporary_selection(selection, {});
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);

    ASSERT_EQ(get_line(modifier.circuit_data().layout,
                       modifier.circuit_data().index.key_index().get(segment_key_0)),
              line_0);
    ASSERT_EQ(get_line(modifier.circuit_data().layout,
                       modifier.circuit_data().index.key_index().get(segment_key_1)),
              line_1);
    ASSERT_EQ(get_line(modifier.circuit_data().layout,
                       modifier.circuit_data().index.key_index().get(segment_key_2)),
              line_2);
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
    const auto segment_key_0 =
        modifier.circuit_data().index.key_index().get(segment_part_0.segment);
    const auto segment_key_1 =
        modifier.circuit_data().index.key_index().get(segment_part_1.segment);
    {
        auto guard = ModifierSelectionGuard {modifier};
        modifier.add_to_selection(guard.selection_id(), segment_part_0);
        modifier.add_to_selection(guard.selection_id(), segment_part_1);
        const auto& selection =
            modifier.circuit_data().selection_store.at(guard.selection_id());
        modifier.regularize_temporary_selection(selection, std::vector {point_t {5, 0}});
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);
    // print(modifier.circuit_data().history);
    // print(modifier.circuit_data().layout);
    // print(layout);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);

    ASSERT_EQ(get_line(modifier.circuit_data().layout,
                       modifier.circuit_data().index.key_index().get(segment_key_0)),
              line_0);
    ASSERT_EQ(get_line(modifier.circuit_data().layout,
                       modifier.circuit_data().index.key_index().get(segment_key_1)),
              line_1);
}

}  // namespace editable_circuit

}  // namespace logicsim
