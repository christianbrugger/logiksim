
#include "test_core/editable_circuit/modifier/test_helpers.h"

#include "core/component/editable_circuit/editing/edit_wire.h"
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
    const auto segment_key = modifier.circuit_data().index.key_index().get(segment);
    {
        auto segment_part_0 = segment_part;
        modifier.delete_temporary_wire_segment(segment_part_0);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_THROW(
        static_cast<void>(modifier.circuit_data().index.key_index().get(segment)),
        std::runtime_error);
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(segment_key, modifier.circuit_data().index.key_index().get(segment));
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
}

TEST(EditableCircuitWireHistory, DeleteFullCrosspoint) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::cross_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {0, 10}};

    auto modifier = get_modifier_with_history(layout);
    const auto segment_key = modifier.circuit_data().index.key_index().get(segment);
    {
        auto segment_part_0 = segment_part;
        modifier.delete_temporary_wire_segment(segment_part_0);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(get_segment_count(modifier.circuit_data().layout), 0);
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(segment_key, modifier.circuit_data().index.key_index().get(segment));
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
}

TEST(EditableCircuitWireHistory, DeletePartialFront) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {0, 5}};

    auto modifier = get_modifier_with_history(layout);
    const auto segment_key = modifier.circuit_data().index.key_index().get(segment);
    {
        auto segment_part_0 = segment_part;
        modifier.delete_temporary_wire_segment(segment_part_0);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(get_segment_count(modifier.circuit_data().layout), 1);
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key, modifier.circuit_data().index.key_index().get(segment));
}

TEST(EditableCircuitWireHistory, DeletePartialEnd) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {5, 10}};

    auto modifier = get_modifier_with_history(layout);
    const auto segment_key = modifier.circuit_data().index.key_index().get(segment);
    {
        auto segment_part_0 = segment_part;
        modifier.delete_temporary_wire_segment(segment_part_0);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(get_segment_count(modifier.circuit_data().layout), 1);
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key, modifier.circuit_data().index.key_index().get(segment));
}

TEST(EditableCircuitWireHistory, DeletePartialMiddle) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {3, 6}};

    auto modifier = get_modifier_with_history(layout);
    const auto segment_key = modifier.circuit_data().index.key_index().get(segment);
    {
        auto segment_part_0 = segment_part;
        modifier.delete_temporary_wire_segment(segment_part_0);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(get_segment_count(modifier.circuit_data().layout), 2);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key, modifier.circuit_data().index.key_index().get(segment));
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
    const auto segment_key = modifier.circuit_data().index.key_index().get(segment);
    {
        auto segment_part_0 = segment_part;
        modifier.move_or_delete_temporary_wire(segment_part_0, move_delta_t {10, 10});
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key, modifier.circuit_data().index.key_index().get(segment));
}

TEST(EditableCircuitWireHistory, MovePartialMiddle) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {2, 7}};

    auto modifier = get_modifier_with_history(layout);
    const auto segment_key = modifier.circuit_data().index.key_index().get(segment);
    {
        auto segment_part_0 = segment_part;
        modifier.move_or_delete_temporary_wire(segment_part_0, move_delta_t {10, 10});
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key, modifier.circuit_data().index.key_index().get(segment));
}

TEST(EditableCircuitWireHistory, MovePartialDelete) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {2, 7}};

    auto modifier = get_modifier_with_history(layout);
    const auto segment_key = modifier.circuit_data().index.key_index().get(segment);
    {
        auto segment_part_0 = segment_part;
        const auto overflow = int {offset_t::max()};
        modifier.move_or_delete_temporary_wire(segment_part_0,
                                               move_delta_t {overflow, overflow});
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
// Create
//

TEST(EditableCircuitWireHistory, AddTemporary) {
    auto layout = Layout {};

    auto modifier = get_modifier_with_history(layout);
    modifier.add_wire_segment(ordered_line_t {point_t {0, 0}, point_t {10, 0}},
                              InsertionMode::temporary);
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
}

}  // namespace editable_circuit

}  // namespace logicsim
