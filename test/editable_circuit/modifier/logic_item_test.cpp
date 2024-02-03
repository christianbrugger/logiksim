
#include "./test_helpers.h"
#include "component/editable_circuit/editing/edit_logicitem.h"
#include "component/editable_circuit/modifier.h"
#include "format/container.h"
#include "layout.h"
#include "layout_message.h"
#include "logging.h"
#include "vocabulary/logicitem_definition.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

namespace editable_circuit {

//
// Test Construction
//

TEST(EditableCircuitModifierLogicItem, DefaultConstruction) {
    auto modifier = Modifier {};

    ASSERT_EQ(modifier.circuit_data().layout.empty(), true);
    ASSERT_EQ(modifier.circuit_data().store_messages, false);
    ASSERT_EQ(modifier.circuit_data().messages.empty(), true);
}

TEST(EditableCircuitModifierLogicItem, ConstructionWithLayout) {
    auto layout = Layout {};
    add_and_element(layout, display_state_t::normal);

    auto modifier = Modifier {Layout {layout}};

    ASSERT_EQ(modifier.circuit_data().layout.empty(), false);
    ASSERT_EQ(modifier.circuit_data().layout, layout);
}

//
// Test Logging
//

TEST(EditableCircuitModifierLogicItem, VerifyLogging) {
    auto modifier = Modifier {ModifierConfig {.store_messages = true}};

    ASSERT_EQ(modifier.circuit_data().layout.empty(), true);
    ASSERT_EQ(modifier.circuit_data().store_messages, true);
    ASSERT_EQ(modifier.circuit_data().messages.empty(), true);

    modifier.add_wire_segment(ordered_line_t {point_t {0, 0}, point_t {10, 0}},
                              InsertionMode::insert_or_discard);
    ASSERT_EQ(modifier.circuit_data().messages.empty(), false);
}

TEST(EditableCircuitModifierLogicItem, VerifyNoLogging) {
    auto modifier = Modifier {};

    ASSERT_EQ(modifier.circuit_data().layout.empty(), true);
    ASSERT_EQ(modifier.circuit_data().store_messages, false);
    ASSERT_EQ(modifier.circuit_data().messages.empty(), true);

    modifier.add_wire_segment(ordered_line_t {point_t {0, 0}, point_t {10, 0}},
                              InsertionMode::insert_or_discard);
    ASSERT_EQ(modifier.circuit_data().messages.empty(), true);
}

//
// swap_and_delete_single_element
//

TEST(EditableCircuitModifierLogicItem, DeleteTemporaryElement) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;

    auto layout = Layout {};
    auto logicitem_id = add_and_element(layout, temporary);

    ASSERT_EQ(logicitem_id, logicitem_id_t {0});

    auto modifier = Modifier {Layout {layout}, ModifierConfig {.store_messages = true}};
    auto preserved_id = logicitem_id_t {0};
    modifier.delete_temporary_logicitem(logicitem_id, &preserved_id);

    // logicitem_ids
    ASSERT_EQ(logicitem_id, null_logicitem_id);
    ASSERT_EQ(preserved_id, null_logicitem_id);

    // layout
    ASSERT_EQ(modifier.circuit_data().layout.empty(), true);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.size(), 1);
    ASSERT_EQ(modifier.circuit_data().messages.at(0),
              Message {LogicItemDeleted {logicitem_id_t {0}}});
}

TEST(EditableCircuitModifierLogicItem, DeletePreserving1) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;

    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    auto logicitem_id_1 =
        add_and_element(layout, temporary, connection_count_t {3}, point_t {2, 2});

    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});
    ASSERT_EQ(logicitem_id_1, logicitem_id_t {1});

    auto modifier = Modifier {Layout {layout}, ModifierConfig {.store_messages = true}};
    modifier.delete_temporary_logicitem(logicitem_id_0, &logicitem_id_1);

    // logicitem_ids
    ASSERT_EQ(logicitem_id_0, null_logicitem_id);
    ASSERT_EQ(logicitem_id_1, logicitem_id_t {0});

    // layout
    assert_logicitem_count(modifier, 1);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {3},
                           point_t {2, 2});

    // messages
    const auto message0 = Message {LogicItemDeleted {logicitem_id_t {0}}};
    const auto message1 = Message {LogicItemIdUpdated {
        .new_logicitem_id = logicitem_id_t {0},
        .old_logicitem_id = logicitem_id_t {1},
    }};
    ASSERT_EQ(modifier.circuit_data().messages.size(), 2);
    ASSERT_EQ(modifier.circuit_data().messages.at(0), message0);
    ASSERT_EQ(modifier.circuit_data().messages.at(1), message1);
}

TEST(EditableCircuitModifierLogicItem, DeletePreserving2) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;

    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    auto logicitem_id_1 =
        add_and_element(layout, temporary, connection_count_t {3}, point_t {2, 2});
    auto logicitem_id_2 =
        add_and_element(layout, valid, connection_count_t {5}, point_t {4, 4});

    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});
    ASSERT_EQ(logicitem_id_1, logicitem_id_t {1});
    ASSERT_EQ(logicitem_id_2, logicitem_id_t {2});

    auto modifier = Modifier {Layout {layout}, ModifierConfig {.store_messages = true}};
    modifier.delete_temporary_logicitem(logicitem_id_1, &logicitem_id_0);

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});
    ASSERT_EQ(logicitem_id_1, null_logicitem_id);

    // layout
    assert_logicitem_count(modifier, 2);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    assert_logicitem_equal(modifier, logicitem_id_t {1}, connection_count_t {5},
                           point_t {4, 4});

    // messages
    const auto message0 = Message {LogicItemDeleted {logicitem_id_t {1}}};
    const auto message1 = Message {LogicItemIdUpdated {
        .new_logicitem_id = logicitem_id_t {1},
        .old_logicitem_id = logicitem_id_t {2},
    }};
    const auto message2 = Message {InsertedLogicItemIdUpdated {
        .new_logicitem_id = logicitem_id_t {1},
        .old_logicitem_id = logicitem_id_t {2},
        .data = to_layout_calculation_data(modifier.circuit_data().layout,
                                           logicitem_id_t {1}),
    }};
    ASSERT_EQ(modifier.circuit_data().messages.size(), 3);
    ASSERT_EQ(modifier.circuit_data().messages.at(0), message0);
    ASSERT_EQ(modifier.circuit_data().messages.at(1), message1);
    ASSERT_EQ(modifier.circuit_data().messages.at(2), message2);
}

//
// is_logic_item_position_representable
//

TEST(EditableCircuitModifierLogicItem, IsRepresentableAndElement) {
    using namespace editing;
    ;

    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {0, 0});

    constexpr static auto overflow = int {grid_t::max()} + 100;

    // true
    ASSERT_EQ(is_logicitem_position_representable(layout, logicitem_id_0, 10, 10), true);
    ASSERT_EQ(is_logicitem_position_representable(layout, logicitem_id_0, -10, -10),
              true);

    // false
    ASSERT_EQ(is_logicitem_position_representable(layout, logicitem_id_0, overflow, 10),
              false);
    ASSERT_EQ(is_logicitem_position_representable(layout, logicitem_id_0, -overflow, 10),
              false);
    ASSERT_EQ(is_logicitem_position_representable(layout, logicitem_id_0, 0, overflow),
              false);
    ASSERT_EQ(is_logicitem_position_representable(layout, logicitem_id_0, 0, -overflow),
              false);
}

//
// move_or_delete_logic_item
//

TEST(EditableCircuitModifierLogicItem, MoveLogicItemSuccess) {
    using namespace editable_circuit::info_message;

    using enum display_state_t;

    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto modifier = Modifier {Layout {layout}, ModifierConfig {.store_messages = true}};
    modifier.move_or_delete_temporary_logicitem(logicitem_id_0, 9, -11);

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(modifier, 1);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {2},
                           point_t {10, -10});

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.size(), 0);
}

TEST(EditableCircuitModifierLogicItem, MoveLogicItemUnchecked) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto modifier = Modifier {Layout {layout}, ModifierConfig {.store_messages = true}};
    modifier.move_temporary_logicitem_unchecked(logicitem_id_0, 9, -11);

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(modifier, 1);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {2},
                           point_t {10, -10});

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.size(), 0);
}

TEST(EditableCircuitModifierLogicItem, MoveLogicItemDeleted) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto modifier = Modifier {Layout {layout}, ModifierConfig {.store_messages = true}};
    constexpr static auto overflow = int {grid_t::max()} + 100;
    modifier.move_or_delete_temporary_logicitem(logicitem_id_0, overflow, 0);

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, null_logicitem_id);

    // layout
    assert_logicitem_count(modifier, 0);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.size(), 1);
    const auto m0 = Message {LogicItemDeleted {logicitem_id_t {0}}};
    ASSERT_EQ(modifier.circuit_data().messages.at(0), m0);
}

//
// change_logic_item_insertion_mode  forward
//

TEST(EditableCircuitModifierLogicItem, LogicItemChangeModeToTempValid) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto modifier = Modifier {Layout {layout}, ModifierConfig {.store_messages = true}};
    modifier.change_logicitem_insertion_mode(logicitem_id_0, InsertionMode::collisions);

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(modifier, 1);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    ASSERT_EQ(get_display_state(modifier, logicitem_id_t {0}), valid);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.size(), 1);

    const auto m0 = Message {LogicItemInserted {
        .logicitem_id = logicitem_id_t {0},
        .data = to_layout_calculation_data(layout, logicitem_id_t {0}),
    }};
    ASSERT_EQ(modifier.circuit_data().messages.at(0), m0);
}

TEST(EditableCircuitModifierLogicItem, LogicItemChangeModeToInsert) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto modifier = Modifier {Layout {layout}, ModifierConfig {.store_messages = true}};
    modifier.change_logicitem_insertion_mode(logicitem_id_0,
                                             InsertionMode::insert_or_discard);

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(modifier, 1);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    ASSERT_EQ(get_display_state(modifier, logicitem_id_t {0}), normal);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.size(), 1);
    const auto m0 = Message {
        LogicItemInserted {.logicitem_id = logicitem_id_t {0},
                           .data = to_layout_calculation_data(
                               modifier.circuit_data().layout, logicitem_id_t {0})}};
    ASSERT_EQ(modifier.circuit_data().messages.at(0), m0);
}

TEST(EditableCircuitModifierLogicItem, LogicItemChangeModeToTempColliding) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, normal, connection_count_t {2}, point_t {1, 1});
    auto logicitem_id_1 =
        add_and_element(layout, temporary, connection_count_t {3}, point_t {2, 2});
    assert_logicitem_count(layout, 2);
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});
    ASSERT_EQ(logicitem_id_1, logicitem_id_t {1});

    auto modifier = Modifier {Layout {layout}, ModifierConfig {.store_messages = true}};
    modifier.change_logicitem_insertion_mode(logicitem_id_1, InsertionMode::collisions);

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_1, logicitem_id_t {1});

    // layout
    assert_logicitem_count(modifier, 2);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    assert_logicitem_equal(modifier, logicitem_id_t {1}, connection_count_t {3},
                           point_t {2, 2});
    ASSERT_EQ(get_display_state(modifier, logicitem_id_t {0}), normal);
    ASSERT_EQ(get_display_state(modifier, logicitem_id_t {1}), colliding);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.size(), 0);
}

TEST(EditableCircuitModifierLogicItem, LogicItemChangeModeToDiscard) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, normal, connection_count_t {2}, point_t {1, 1});
    auto logicitem_id_1 =
        add_and_element(layout, temporary, connection_count_t {3}, point_t {2, 2});
    assert_logicitem_count(layout, 2);
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});
    ASSERT_EQ(logicitem_id_1, logicitem_id_t {1});

    auto modifier = Modifier {Layout {layout}, ModifierConfig {.store_messages = true}};
    modifier.change_logicitem_insertion_mode(logicitem_id_1,
                                             InsertionMode::insert_or_discard);

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_1, null_logicitem_id);

    // layout
    assert_logicitem_count(modifier, 1);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    ASSERT_EQ(get_display_state(modifier, logicitem_id_t {0}), normal);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.size(), 1);
    const auto message0 = Message {LogicItemDeleted {logicitem_id_t {1}}};
    ASSERT_EQ(modifier.circuit_data().messages.at(0), message0);
}

//
// change_logic_item_insertion_mode  backwards
//

TEST(EditableCircuitModifierLogicItem, LogicItemChangeModeBToValid) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, normal, connection_count_t {2}, point_t {1, 1});
    assert_logicitem_count(layout, 1);
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto modifier = Modifier {Layout {layout}, ModifierConfig {.store_messages = true}};
    modifier.change_logicitem_insertion_mode(logicitem_id_0, InsertionMode::collisions);

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(modifier, 1);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    ASSERT_EQ(get_display_state(modifier, logicitem_id_t {0}), valid);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.size(), 0);
}

TEST(EditableCircuitModifierLogicItem, LogicItemChangeModeBToTemporary) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, normal, connection_count_t {2}, point_t {1, 1});
    assert_logicitem_count(layout, 1);
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto modifier = Modifier {Layout {layout}, ModifierConfig {.store_messages = true}};
    modifier.change_logicitem_insertion_mode(logicitem_id_0, InsertionMode::temporary);

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(modifier, 1);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    ASSERT_EQ(get_display_state(modifier, logicitem_id_t {0}), temporary);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.size(), 1);
    const auto m0 = Message {
        LogicItemUninserted {.logicitem_id = logicitem_id_t {0},
                             .data = to_layout_calculation_data(
                                 modifier.circuit_data().layout, logicitem_id_t {0})}};
    ASSERT_EQ(modifier.circuit_data().messages.at(0), m0);
}

TEST(EditableCircuitModifierLogicItem, LogicItemChangeModeBToTemporaryPreserving) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, normal, connection_count_t {2}, point_t {1, 1});

    assert_logicitem_count(layout, 1);
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});
    const auto data0 = to_layout_calculation_data(layout, logicitem_id_t {0});

    auto modifier = Modifier {Layout {layout}, ModifierConfig {.store_messages = true}};
    modifier.change_logicitem_insertion_mode(logicitem_id_0, InsertionMode::temporary);

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(modifier, 1);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    ASSERT_EQ(get_display_state(modifier, logicitem_id_t {0}), temporary);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.size(), 1);

    const auto m0 = Message {LogicItemUninserted {
        .logicitem_id = logicitem_id_t {0},
        .data = data0,
    }};
    ASSERT_EQ(modifier.circuit_data().messages.at(0), m0);
}

//
// add_standard_logic_item
//

TEST(EditableCircuitModifierLogicItem, LogicItemAddElement) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto modifier = Modifier {Layout {layout}, ModifierConfig {.store_messages = true}};

    const auto definition = LogicItemDefinition {
        .logicitem_type = LogicItemType::xor_element,
        .input_count = connection_count_t {7},
        .output_count = connection_count_t {1},
        .orientation = orientation_t::right,
    };
    const auto logicitem_id = modifier.add_logicitem(definition, point_t {2, 3},
                                                     InsertionMode::insert_or_discard);

    //  logicitem_ids
    ASSERT_EQ(logicitem_id, logicitem_id_t {0});

    // layout
    assert_logicitem_count(modifier, 1);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {7},
                           point_t {2, 3});
    ASSERT_EQ(get_display_state(modifier, logicitem_id_t {0}), normal);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.size(), 2);
    const auto m0 = Message {LogicItemCreated {logicitem_id_t {0}}};
    const auto m1 = Message {
        LogicItemInserted {.logicitem_id = logicitem_id_t {0},
                           .data = to_layout_calculation_data(
                               modifier.circuit_data().layout, logicitem_id_t {0})}};
    ASSERT_EQ(modifier.circuit_data().messages.at(0), m0);
    ASSERT_EQ(modifier.circuit_data().messages.at(1), m1);
}

//
// logic item combinations
//

auto add_xor_element(Modifier &modifier, point_t position, InsertionMode insertion_mode)
    -> logicitem_id_t {
    const auto definition = LogicItemDefinition {
        .logicitem_type = LogicItemType::xor_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .orientation = orientation_t::right,
    };
    return modifier.add_logicitem(definition, position, insertion_mode);
}

TEST(EditableCircuitModifierLogicItem, LogicItemCombineAddMoveDelete) {
    using enum InsertionMode;
    auto layout = Layout {};
    auto modifier = Modifier {Layout {layout}, ModifierConfig {.store_messages = true}};

    auto id_0 = add_xor_element(modifier, point_t {1, 1}, temporary);
    auto id_1 = add_xor_element(modifier, point_t {10, 10}, insert_or_discard);

    modifier.move_or_delete_temporary_logicitem(id_0, 10, 10);

    modifier.change_logicitem_insertion_mode(id_0, collisions);
    ASSERT_EQ(get_display_state(modifier, id_0), display_state_t::colliding);

    modifier.change_logicitem_insertion_mode(id_0, insert_or_discard);
    ASSERT_EQ(id_0, null_logicitem_id);

    id_1 = logicitem_id_t {0};
    modifier.change_logicitem_insertion_mode(id_1, temporary);

    modifier.delete_temporary_logicitem(id_1);
    ASSERT_EQ(id_1, null_logicitem_id);

    // layout
    assert_logicitem_count(modifier, 0);
}

}  // namespace editable_circuit

}  // namespace logicsim