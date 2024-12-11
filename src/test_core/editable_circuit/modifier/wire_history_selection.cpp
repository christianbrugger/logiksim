
#include "test_core/editable_circuit/modifier/test_helpers.h"

#include "core/component/editable_circuit/editing/edit_wire.h"
#include "core/component/editable_circuit/key_state.h"
#include "core/component/editable_circuit/modifier.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

namespace editable_circuit {

TEST(EditableCircuitWireHistory, SelectionInsertRestore) {
    // prepare
    auto layout = Layout {};
    const auto segment_index =
        add_to_wire(layout, temporary_wire_id, SegmentPointType::shadow_point,
                    ordered_line_t {point_t {0, 0}, point_t {0, 10}});
    const auto segment = segment_t {temporary_wire_id, segment_index};
    auto selection = Selection {};
    selection.add_segment(segment_part_t {segment, part_t {0, 10}});

    auto modifier = get_modifier(layout);
    modifier.set_visible_selection(selection);
    modifier.enable_history();
    const auto state_0 = layout_key_state_t {modifier};
    {
        auto segment_part = segment_part_t {segment, part_t {0, 5}};
        modifier.change_wire_insertion_mode(segment_part,
                                            InsertionMode::insert_or_discard);
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
