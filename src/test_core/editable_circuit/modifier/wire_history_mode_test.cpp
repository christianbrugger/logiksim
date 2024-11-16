

#include "test_core/editable_circuit/modifier/test_helpers.h"

#include "core/component/editable_circuit/editing/edit_wire.h"
#include "core/component/editable_circuit/modifier.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

namespace editable_circuit {

//
// Temporary to Colliding
//

TEST(EditableCircuitWireHistory, TemporaryToCollidingFull) {
    auto layout = Layout {};
    add_test_wire(layout, SegmentPointType::output,
                  std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {0, 10}};

    auto modifier = get_modifier_with_history(layout);
    const auto segment_key = modifier.circuit_data().index.key_index().get(segment);
    {
        auto segment_part_0 = segment_part;
        modifier.change_wire_insertion_mode(segment_part_0, InsertionMode::collisions);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key, modifier.circuit_data().index.key_index().get(segment));
}

TEST(EditableCircuitWireHistory, TemporaryToCollidingPartialMiddle) {
    auto layout = Layout {};
    add_test_wire(layout, SegmentPointType::output,
                  std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {3, 7}};

    auto modifier = get_modifier_with_history(layout);
    const auto segment_key = modifier.circuit_data().index.key_index().get(segment);
    {
        auto segment_part_0 = segment_part;
        modifier.change_wire_insertion_mode(segment_part_0, InsertionMode::collisions);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key, modifier.circuit_data().index.key_index().get(segment));
}

TEST(EditableCircuitWireHistory, TemporaryToCollidingPartialSideCrosspoint) {
    auto layout = Layout {};
    add_test_wire(layout, SegmentPointType::output,
                  std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::cross_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {5, 10}};

    auto modifier = get_modifier_with_history(layout);
    const auto segment_key = modifier.circuit_data().index.key_index().get(segment);
    {
        auto segment_part_0 = segment_part;
        modifier.change_wire_insertion_mode(segment_part_0, InsertionMode::collisions);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key, modifier.circuit_data().index.key_index().get(segment));
}

TEST(EditableCircuitWireHistory, TemporaryToValidFull) {
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
        modifier.change_wire_insertion_mode(segment_part_0, InsertionMode::collisions);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key, modifier.circuit_data().index.key_index().get(segment));
}

TEST(EditableCircuitWireHistory, TemporaryToValidPartialCrosspoint) {
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::cross_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {0, 5}};

    auto modifier = get_modifier_with_history(layout);
    const auto segment_key = modifier.circuit_data().index.key_index().get(segment);
    {
        auto segment_part_0 = segment_part;
        modifier.change_wire_insertion_mode(segment_part_0, InsertionMode::collisions);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key, modifier.circuit_data().index.key_index().get(segment));
}

TEST(EditableCircuitWireHistory, TemporaryToValidPartialMerged) {
    auto layout = Layout {};
    add_test_wire(layout, SegmentPointType::output,
                  std::array {ordered_line_t {point_t {0, -5}, point_t {0, 5}}});
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::cross_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {0, 5}};

    auto modifier = get_modifier_with_history(layout);
    const auto segment_key = modifier.circuit_data().index.key_index().get(segment);
    {
        auto segment_part_0 = segment_part;
        modifier.change_wire_insertion_mode(segment_part_0, InsertionMode::collisions);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);
    ASSERT_EQ(modifier.circuit_data().layout.wires().size(), 3);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key, modifier.circuit_data().index.key_index().get(segment));
}

TEST(EditableCircuitWireHistory, TemporaryToValidPartialMergedTwoSides) {
    auto layout = Layout {};
    add_test_wire(layout, SegmentPointType::output,
                  std::array {ordered_line_t {point_t {0, -5}, point_t {0, 5}}});
    add_test_wire(layout, SegmentPointType::output,
                  std::array {ordered_line_t {point_t {7, -5}, point_t {7, 5}}});
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto segment_part = segment_part_t {segment, part_t {0, 7}};

    auto modifier = get_modifier_with_history(layout);
    const auto segment_key = modifier.circuit_data().index.key_index().get(segment);
    {
        auto segment_part_0 = segment_part;
        modifier.change_wire_insertion_mode(segment_part_0, InsertionMode::collisions);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);
    ASSERT_EQ(modifier.circuit_data().layout.wires().size(), 3);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key, modifier.circuit_data().index.key_index().get(segment));
}

//
// Colliding To Insert
//

TEST(EditableCircuitWireHistory, CollidingToInsertValidFull) {
    // setup
    auto layout = Layout {};
    add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    auto modifier = get_modifier(layout);
    auto segment_part = segment_part_t {
        segment_t {temporary_wire_id, segment_index_t {0}}, part_t {0, 10}};
    modifier.change_wire_insertion_mode(segment_part, InsertionMode::collisions);
    layout = modifier.circuit_data().layout;

    // enable history
    modifier.enable_history();
    const auto segment_key =
        modifier.circuit_data().index.key_index().get(segment_part.segment);
    {
        auto segment_part_0 = segment_part;
        modifier.change_wire_insertion_mode(segment_part_0,
                                            InsertionMode::insert_or_discard);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key,
              modifier.circuit_data().index.key_index().get(segment_part.segment));
}

TEST(EditableCircuitWireHistory, CollidingToInsertValidPartial) {
    // setup
    auto layout = Layout {};
    add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    auto modifier = get_modifier(layout);
    auto segment_part = segment_part_t {
        segment_t {temporary_wire_id, segment_index_t {0}}, part_t {0, 10}};
    modifier.change_wire_insertion_mode(segment_part, InsertionMode::collisions);
    layout = modifier.circuit_data().layout;

    // enable history
    modifier.enable_history();
    const auto segment_key =
        modifier.circuit_data().index.key_index().get(segment_part.segment);
    {
        auto segment_part_0 = segment_part_t {segment_part.segment, part_t {2, 7}};
        modifier.change_wire_insertion_mode(segment_part_0,
                                            InsertionMode::insert_or_discard);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key,
              modifier.circuit_data().index.key_index().get(segment_part.segment));
}

TEST(EditableCircuitWireHistory, CollidingToInsertCollisionsFull) {
    // setup
    auto layout = Layout {};
    add_test_wire(layout, SegmentPointType::output,
                  std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    auto modifier = get_modifier(layout);
    auto segment_part = segment_part_t {
        segment_t {temporary_wire_id, segment_index_t {0}}, part_t {0, 10}};
    modifier.change_wire_insertion_mode(segment_part, InsertionMode::collisions);
    layout = modifier.circuit_data().layout;

    // enable history
    modifier.enable_history();
    const auto segment_key =
        modifier.circuit_data().index.key_index().get(segment_part.segment);
    {
        auto segment_part_0 = segment_part;
        modifier.change_wire_insertion_mode(segment_part_0,
                                            InsertionMode::insert_or_discard);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key,
              modifier.circuit_data().index.key_index().get(segment_part.segment));
}

TEST(EditableCircuitWireHistory, CollidingToInsertCollisionsPartial) {
    // setup
    auto layout = Layout {};
    add_test_wire(layout, SegmentPointType::output,
                  std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    auto modifier = get_modifier(layout);
    auto segment_part = segment_part_t {
        segment_t {temporary_wire_id, segment_index_t {0}}, part_t {0, 10}};
    modifier.change_wire_insertion_mode(segment_part, InsertionMode::collisions);
    layout = modifier.circuit_data().layout;

    // enable history
    modifier.enable_history();
    const auto segment_key =
        modifier.circuit_data().index.key_index().get(segment_part.segment);
    {
        auto segment_part_0 = segment_part_t {segment_part.segment, part_t {5, 10}};
        modifier.change_wire_insertion_mode(segment_part_0,
                                            InsertionMode::insert_or_discard);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key,
              modifier.circuit_data().index.key_index().get(segment_part.segment));
}

//
// Insert to Colliding
//

TEST(EditableCircuitWireHistory, InsertToCollidingFull) {
    // setup
    auto layout = Layout {};
    add_test_wire(layout, SegmentPointType::output,
                  std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    const auto segment = segment_t {wire_id_t {2}, segment_index_t {0}};
    const auto segment_part = segment_part_t {segment, part_t {0, 10}};

    // enable history
    auto modifier = get_modifier_with_history(layout);
    const auto segment_key =
        modifier.circuit_data().index.key_index().get(segment_part.segment);
    {
        auto segment_part_0 = segment_part;
        modifier.change_wire_insertion_mode(segment_part_0, InsertionMode::collisions);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key,
              modifier.circuit_data().index.key_index().get(segment_part.segment));
}

TEST(EditableCircuitWireHistory, InsertToCollidingPartial) {
    // setup
    auto layout = Layout {};
    add_test_wire(layout, SegmentPointType::output,
                  std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    const auto segment = segment_t {wire_id_t {2}, segment_index_t {0}};
    const auto segment_part = segment_part_t {segment, part_t {0, 5}};

    // enable history
    auto modifier = get_modifier_with_history(layout);
    const auto segment_key =
        modifier.circuit_data().index.key_index().get(segment_part.segment);
    {
        auto segment_part_0 = segment_part;
        modifier.change_wire_insertion_mode(segment_part_0, InsertionMode::collisions);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key,
              modifier.circuit_data().index.key_index().get(segment_part.segment));
}

//
// Colliding To Temporary
//

TEST(EditableCircuitWireHistory, ValidToTemporaryFull) {
    // setup
    auto layout = Layout {};
    add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    auto modifier = get_modifier(layout);
    auto segment_part = segment_part_t {
        segment_t {temporary_wire_id, segment_index_t {0}}, part_t {0, 10}};
    modifier.change_wire_insertion_mode(segment_part, InsertionMode::collisions);
    layout = modifier.circuit_data().layout;
    // setup

    // enable history
    modifier.enable_history();
    const auto segment_key =
        modifier.circuit_data().index.key_index().get(segment_part.segment);
    {
        auto segment_part_0 = segment_part;
        modifier.change_wire_insertion_mode(segment_part_0, InsertionMode::temporary);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key,
              modifier.circuit_data().index.key_index().get(segment_part.segment));
}

TEST(EditableCircuitWireHistory, ValidToTemporaryPartial) {
    // setup
    auto layout = Layout {};
    add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    auto modifier = get_modifier(layout);
    auto segment_part = segment_part_t {
        segment_t {temporary_wire_id, segment_index_t {0}}, part_t {0, 10}};
    modifier.change_wire_insertion_mode(segment_part, InsertionMode::collisions);
    layout = modifier.circuit_data().layout;
    // setup

    // enable history
    modifier.enable_history();
    const auto segment_key =
        modifier.circuit_data().index.key_index().get(segment_part.segment);
    {
        auto segment_part_0 = segment_part_t {segment_part.segment, part_t {2, 7}};
        modifier.change_wire_insertion_mode(segment_part_0, InsertionMode::temporary);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key,
              modifier.circuit_data().index.key_index().get(segment_part.segment));
}

TEST(EditableCircuitWireHistory, CollidingToTemporaryFull) {
    // setup
    auto layout = Layout {};
    add_test_wire(layout, SegmentPointType::output,
                  std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    auto modifier = get_modifier(layout);
    auto segment_part = segment_part_t {
        segment_t {temporary_wire_id, segment_index_t {0}}, part_t {0, 10}};
    modifier.change_wire_insertion_mode(segment_part, InsertionMode::collisions);
    layout = modifier.circuit_data().layout;
    // setup

    // enable history
    modifier.enable_history();
    const auto segment_key =
        modifier.circuit_data().index.key_index().get(segment_part.segment);
    {
        auto segment_part_0 = segment_part;
        modifier.change_wire_insertion_mode(segment_part_0, InsertionMode::temporary);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key,
              modifier.circuit_data().index.key_index().get(segment_part.segment));
}

TEST(EditableCircuitWireHistory, CollidingToTemporaryPartial) {
    // setup
    auto layout = Layout {};
    add_test_wire(layout, SegmentPointType::output,
                  std::array {ordered_line_t {point_t {0, 0}, point_t {10, 0}}});
    add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                ordered_line_t {point_t {0, 0}, point_t {10, 0}});
    auto modifier = get_modifier(layout);
    auto segment_part = segment_part_t {
        segment_t {temporary_wire_id, segment_index_t {0}}, part_t {0, 10}};
    modifier.change_wire_insertion_mode(segment_part, InsertionMode::collisions);
    layout = modifier.circuit_data().layout;
    // setup

    // enable history
    modifier.enable_history();
    const auto segment_key =
        modifier.circuit_data().index.key_index().get(segment_part.segment);
    {
        auto segment_part_0 = segment_part_t {segment_part.segment, part_t {2, 7}};
        modifier.change_wire_insertion_mode(segment_part_0, InsertionMode::temporary);
    }
    Expects(is_valid(modifier));

    // before undo
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), false);

    // after undo
    modifier.undo_group();
    ASSERT_EQ(are_normalized_equal(modifier.circuit_data().layout, layout), true);
    ASSERT_EQ(segment_key,
              modifier.circuit_data().index.key_index().get(segment_part.segment));
}

}  // namespace editable_circuit

}  // namespace logicsim
