
#include "test_core/editable_circuit/modifier/test_helpers.h"

#include "core/component/editable_circuit/editing/edit_wire.h"
#include "core/component/editable_circuit/modifier.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

namespace editable_circuit {

TEST(EditableCircuitWireHistory, DeleteFullSegment) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {0, 10}};

    auto modifier = get_modifier_with_history(layout);
    {
        auto segment_part_0 = segment_part;
        modifier.delete_temporary_wire_segment(segment_part_0);
    }
    print(modifier.circuit_data().layout);
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(get_segment_count(modifier.circuit_data().layout), 0);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(get_segment_count(modifier.circuit_data().layout), 1);
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
}

TEST(EditableCircuitWireHistory, DeleteFullSegmentCrosspoint) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::cross_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {0, 10}};

    auto modifier = get_modifier_with_history(layout);
    {
        auto segment_part_0 = segment_part;
        modifier.delete_temporary_wire_segment(segment_part_0);
    }
    print(modifier.circuit_data().layout);
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(get_segment_count(modifier.circuit_data().layout), 0);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(get_segment_count(modifier.circuit_data().layout), 1);
    print(layout);
    print(modifier.circuit_data().layout);
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
}

}  // namespace editable_circuit

}  // namespace logicsim
