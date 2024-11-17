

#include "test_core/editable_circuit/modifier/test_helpers.h"

#include "core/component/editable_circuit/editing/edit_wire.h"
#include "core/component/editable_circuit/modifier.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

namespace editable_circuit {

//
// Split Temporary Segments
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
    auto layout = Layout {};
    const auto segment_index_0 =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {5, 0}});
    const auto segment_index_1 =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {5, 0}, point_t {10, 0}});
    const auto segment_part_0 =
        segment_part_t {segment_t {temporary_wire_id, segment_index_0}, part_t {0, 5}};
    const auto segment_part_1 =
        segment_part_t {segment_t {temporary_wire_id, segment_index_1}, part_t {0, 5}};

    auto modifier = get_modifier_with_history(layout);
    // const auto segment_key_0 =
    //     modifier.circuit_data().index.key_index().get(segment_part_0.segment);
    // const auto segment_key_1 =
    //     modifier.circuit_data().index.key_index().get(segment_part_1.segment);
    {
        auto selection = Selection {};
        selection.add_segment(segment_part_0);
        selection.add_segment(segment_part_1);
        modifier.regularize_temporary_selection(selection, {});
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);
    print(modifier.circuit_data().history);
    print(modifier.circuit_data().layout);
    print(layout);

    // after undo
    modifier.undo_group();
    // ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    // ASSERT_EQ(segment_key_0,
    //           modifier.circuit_data().index.key_index().get(segment_part_0.segment));
    // ASSERT_EQ(segment_key_1,
    //           modifier.circuit_data().index.key_index().get(segment_part_1.segment));
}

}  // namespace editable_circuit

}  // namespace logicsim
