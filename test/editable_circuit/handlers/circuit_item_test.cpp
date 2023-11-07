
#include "./test_helpers.h"
#include "editable_circuit/handler.h"
#include "vocabulary/logicitem_definition.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

//
// Test Setup
//

TEST(EditableCircuitHandler, VerificationSetup) {
    auto layout = Layout {};
    auto setup = HandlerSetup {layout};

    setup.validate();
    ASSERT_EQ(setup.state.layout.empty(), true);
    ASSERT_EQ(setup.recorder.messages().empty(), true);
}

//
// swap_and_delete_single_element
//

TEST(EditableCircuitHandler, DeleteTemporaryElement) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};
    auto logicitem_id = add_and_element(layout, temporary);

    ASSERT_EQ(logicitem_id, logicitem_id_t {0});

    auto setup = HandlerSetup {layout};
    auto preserved_id = logicitem_id_t {0};
    swap_and_delete_logic_item(layout, setup.sender, logicitem_id, &preserved_id);

    setup.validate();

    // logicitem_ids
    ASSERT_EQ(logicitem_id, null_logicitem_id);
    ASSERT_EQ(preserved_id, null_logicitem_id);

    // layout
    ASSERT_EQ(setup.state.layout.empty(), true);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 1);
    ASSERT_EQ(setup.recorder.messages().at(0),
              Message {LogicItemDeleted {logicitem_id_t {0}}});
}

TEST(EditableCircuitHandler, DeletePreserving1) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    auto logicitem_id_1 =
        add_and_element(layout, temporary, connection_count_t {3}, point_t {2, 2});

    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});
    ASSERT_EQ(logicitem_id_1, logicitem_id_t {1});

    auto setup = HandlerSetup {layout};
    swap_and_delete_logic_item(layout, setup.sender, logicitem_id_0, &logicitem_id_1);

    setup.validate();
    // logicitem_ids
    ASSERT_EQ(logicitem_id_0, null_logicitem_id);
    ASSERT_EQ(logicitem_id_1, logicitem_id_t {0});

    // layout
    assert_logicitem_count(layout, 1);
    assert_logicitem_equal(layout, logicitem_id_t {0}, connection_count_t {3},
                           point_t {2, 2});

    // messages
    const auto message0 = Message {LogicItemDeleted {logicitem_id_t {0}}};
    const auto message1 = Message {LogicItemIdUpdated {
        .new_logicitem_id = logicitem_id_t {0},
        .old_logicitem_id = logicitem_id_t {1},
    }};
    ASSERT_EQ(setup.recorder.messages().size(), 2);
    ASSERT_EQ(setup.recorder.messages().at(0), message0);
    ASSERT_EQ(setup.recorder.messages().at(1), message1);
}

TEST(EditableCircuitHandler, DeletePreserving2) {
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

    auto setup = HandlerSetup {layout};
    swap_and_delete_logic_item(layout, setup.sender, logicitem_id_1, &logicitem_id_0);

    setup.validate();
    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});
    ASSERT_EQ(logicitem_id_1, null_logicitem_id);

    // layout
    assert_logicitem_count(layout, 2);
    assert_logicitem_equal(layout, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    assert_logicitem_equal(layout, logicitem_id_t {1}, connection_count_t {5},
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
        .data = to_layout_calculation_data(layout, logicitem_id_t {1}),
    }};
    ASSERT_EQ(setup.recorder.messages().size(), 3);
    ASSERT_EQ(setup.recorder.messages().at(0), message0);
    ASSERT_EQ(setup.recorder.messages().at(1), message1);
    ASSERT_EQ(setup.recorder.messages().at(2), message2);
}

//
// swap_and_delete_multiple_elements
//

/*
TEST(EditableCircuitHandler, DeleteMultipleElements) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};
    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    auto logicitem_id_1 =
        add_and_element(layout, temporary, connection_count_t {3}, point_t {2, 2});
    auto logicitem_id_2 =
        add_and_element(layout, temporary, connection_count_t {4}, point_t {3, 3});  //
    auto logicitem_id_3 =
        add_and_element(layout, temporary, connection_count_t {5}, point_t {4, 4});

    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});
    ASSERT_EQ(logicitem_id_1, logicitem_id_t {1});
    ASSERT_EQ(logicitem_id_2, logicitem_id_t {2});
    ASSERT_EQ(logicitem_id_3, logicitem_id_t {3});

    auto setup = HandlerSetup {layout};
    auto to_delete = {logicitem_id_1, logicitem_id_0, logicitem_id_3};
    auto preserved_id = logicitem_id_2;
    swap_and_delete_multiple_elements(layout, setup.sender, to_delete, &preserved_id);

    setup.validate();

    // logicitem_ids
    ASSERT_EQ(preserved_id, logicitem_id_t {0});

    // layout
    assert_logicitem_count(layout, 1);
    assert_logicitem_equal(layout, logicitem_id_t {0}, connection_count_t {4},
                         point_t {3, 3});

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 5);
    const auto m0 = Message {LogicItemDeleted {logicitem_id_t {3}}};
    const auto m1 = Message {LogicItemDeleted {logicitem_id_t {1}}};
    const auto m2 = Message {LogicItemIdUpdated {.new_logicitem_id = logicitem_id_t {1},
                                                 .old_logicitem_id = logicitem_id_t {2}}};
    const auto m3 = Message {LogicItemDeleted {logicitem_id_t {0}}};
    const auto m4 = Message {LogicItemIdUpdated {.new_logicitem_id = logicitem_id_t {0},
                                                 .old_logicitem_id = logicitem_id_t {1}}};
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
    ASSERT_EQ(setup.recorder.messages().at(1), m1);
    ASSERT_EQ(setup.recorder.messages().at(2), m2);
    ASSERT_EQ(setup.recorder.messages().at(3), m3);
    ASSERT_EQ(setup.recorder.messages().at(4), m4);
}
*/

//
// is_logic_item_position_representable
//

TEST(EditableCircuitHandler, IsRepresentableAndElement) {
    using namespace editable_circuit;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {0, 0});

    constexpr static auto overflow = int {grid_t::max()} + 100;

    // true
    ASSERT_EQ(is_logic_item_position_representable(layout, logicitem_id_0, 10, 10), true);
    ASSERT_EQ(is_logic_item_position_representable(layout, logicitem_id_0, -10, -10), true);

    // false
    ASSERT_EQ(is_logic_item_position_representable(layout, logicitem_id_0, overflow, 10),
              false);
    ASSERT_EQ(is_logic_item_position_representable(layout, logicitem_id_0, -overflow, 10),
              false);
    ASSERT_EQ(is_logic_item_position_representable(layout, logicitem_id_0, 0, overflow),
              false);
    ASSERT_EQ(is_logic_item_position_representable(layout, logicitem_id_0, 0, -overflow),
              false);
}

//
// move_or_delete_logic_item
//

TEST(EditableCircuitHandler, MoveLogicItemSuccess) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto setup = HandlerSetup {layout};
    move_or_delete_logic_item(layout, setup.sender, logicitem_id_0, 9, -11);

    setup.validate();
    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(layout, 1);
    assert_logicitem_equal(layout, logicitem_id_t {0}, connection_count_t {2},
                           point_t {10, -10});

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 0);
}

TEST(EditableCircuitHandler, MoveLogicItemUnchecked) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto setup = HandlerSetup {layout};
    editable_circuit::move_logic_item_unchecked(layout, logicitem_id_0, 9, -11);

    setup.validate();
    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(layout, 1);
    assert_logicitem_equal(layout, logicitem_id_t {0}, connection_count_t {2},
                           point_t {10, -10});

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 0);
}

TEST(EditableCircuitHandler, MoveLogicItemDeleted) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto setup = HandlerSetup {layout};
    constexpr static auto overflow = int {grid_t::max()} + 100;
    move_or_delete_logic_item(layout, setup.sender, logicitem_id_0, overflow, 0);

    setup.validate();
    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, null_logicitem_id);

    // layout
    assert_logicitem_count(layout, 0);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 1);
    const auto m0 = Message {LogicItemDeleted {logicitem_id_t {0}}};
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
}

//
// change_logic_item_insertion_mode  forward
//

TEST(EditableCircuitHandler, LogicItemChangeModeToTempValid) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto setup = HandlerSetup {layout};
    change_logic_item_insertion_mode(setup.state, logicitem_id_0,
                                     InsertionMode::collisions);

    setup.validate();
    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(layout, 1);
    assert_logicitem_equal(layout, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    ASSERT_EQ(layout.logic_items().display_state(logicitem_id_t {0}), valid);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 1);

    const auto m0 = Message {LogicItemInserted {
        .logicitem_id = logicitem_id_t {0},
        .data = to_layout_calculation_data(layout, logicitem_id_t {0}),
    }};
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
}

TEST(EditableCircuitHandler, LogicItemChangeModeToInsert) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto setup = HandlerSetup {layout};
    change_logic_item_insertion_mode(setup.state, logicitem_id_0,
                                     InsertionMode::insert_or_discard);

    setup.validate();
    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(layout, 1);
    assert_logicitem_equal(layout, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    ASSERT_EQ(layout.logic_items().display_state(logicitem_id_t {0}), normal);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 1);
    const auto m0 = Message {LogicItemInserted {
        .logicitem_id = logicitem_id_t {0},
        .data = to_layout_calculation_data(layout, logicitem_id_t {0})}};
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
}

TEST(EditableCircuitHandler, LogicItemChangeModeToTempColliding) {
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

    auto setup = HandlerSetup {layout};
    change_logic_item_insertion_mode(setup.state, logicitem_id_1,
                                     InsertionMode::collisions);

    setup.validate();
    //  logicitem_ids
    ASSERT_EQ(logicitem_id_1, logicitem_id_t {1});

    // layout
    assert_logicitem_count(layout, 2);
    assert_logicitem_equal(layout, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    assert_logicitem_equal(layout, logicitem_id_t {1}, connection_count_t {3},
                           point_t {2, 2});
    ASSERT_EQ(layout.logic_items().display_state(logicitem_id_t {0}), normal);
    ASSERT_EQ(layout.logic_items().display_state(logicitem_id_t {1}), colliding);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 0);
}

TEST(EditableCircuitHandler, LogicItemChangeModeToDiscard) {
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

    auto setup = HandlerSetup {layout};
    change_logic_item_insertion_mode(setup.state, logicitem_id_1,
                                     InsertionMode::insert_or_discard);

    setup.validate();
    //  logicitem_ids
    ASSERT_EQ(logicitem_id_1, null_logicitem_id);

    // layout
    assert_logicitem_count(layout, 1);
    assert_logicitem_equal(layout, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    ASSERT_EQ(layout.logic_items().display_state(logicitem_id_t {0}), normal);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 1);
    const auto message0 = Message {LogicItemDeleted {logicitem_id_t {1}}};
    ASSERT_EQ(setup.recorder.messages().at(0), message0);
}

//
// change_logic_item_insertion_mode  backwards
//

TEST(EditableCircuitHandler, LogicItemChangeModeBToValid) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, normal, connection_count_t {2}, point_t {1, 1});
    assert_logicitem_count(layout, 1);
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto setup = HandlerSetup {layout};
    change_logic_item_insertion_mode(setup.state, logicitem_id_0,
                                     InsertionMode::collisions);

    setup.validate();
    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(layout, 1);
    assert_logicitem_equal(layout, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    ASSERT_EQ(layout.logic_items().display_state(logicitem_id_t {0}), valid);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 0);
}

TEST(EditableCircuitHandler, LogicItemChangeModeBToTemporary) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, normal, connection_count_t {2}, point_t {1, 1});
    assert_logicitem_count(layout, 1);
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto setup = HandlerSetup {layout};
    change_logic_item_insertion_mode(setup.state, logicitem_id_0, InsertionMode::temporary);

    setup.validate();
    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(layout, 1);
    assert_logicitem_equal(layout, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    ASSERT_EQ(layout.logic_items().display_state(logicitem_id_t {0}), temporary);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 1);
    const auto m0 = Message {LogicItemUninserted {
        .logicitem_id = logicitem_id_t {0},
        .data = to_layout_calculation_data(layout, logicitem_id_t {0})}};
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
}

TEST(EditableCircuitHandler, LogicItemChangeModeBToTemporaryPreserving) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, normal, connection_count_t {2}, point_t {1, 1});

    assert_logicitem_count(layout, 1);
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});
    const auto data0 = to_layout_calculation_data(layout, logicitem_id_t {0});

    auto setup = HandlerSetup {layout};
    change_logic_item_insertion_mode(setup.state, logicitem_id_0, InsertionMode::temporary);

    setup.validate();
    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(layout, 1);
    assert_logicitem_equal(layout, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    ASSERT_EQ(layout.logic_items().display_state(logicitem_id_t {0}), temporary);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 1);

    const auto m0 = Message {LogicItemUninserted {
        .logicitem_id = logicitem_id_t {0},
        .data = data0,
    }};
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
}

//
// add_standard_logic_item
//

TEST(EditableCircuitHandler, LogicItemAddElement) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto setup = HandlerSetup {layout};

    const auto definition = LogicItemDefinition {
        .logicitem_type = LogicItemType::xor_element,
        .input_count = connection_count_t {7},
        .output_count = connection_count_t {1},
        .orientation = orientation_t::right,
    };
    const auto logicitem_id = add_logic_item(setup.state, definition, point_t {2, 3},
                                           InsertionMode::insert_or_discard);

    setup.validate();
    //  logicitem_ids
    ASSERT_EQ(logicitem_id, logicitem_id_t {0});

    // layout
    assert_logicitem_count(layout, 1);
    assert_logicitem_equal(layout, logicitem_id_t {0}, connection_count_t {7},
                           point_t {2, 3});
    ASSERT_EQ(layout.logic_items().display_state(logicitem_id_t {0}), normal);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 2);
    const auto m0 = Message {LogicItemCreated {logicitem_id_t {0}}};
    const auto m1 = Message {LogicItemInserted {
        .logicitem_id = logicitem_id_t {0},
        .data = to_layout_calculation_data(layout, logicitem_id_t {0})}};
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
    ASSERT_EQ(setup.recorder.messages().at(1), m1);
}

//
// logic item combinations
//

auto add_xor_element(editable_circuit::State &state, point_t position,
                     InsertionMode insertion_mode) -> logicitem_id_t {
    const auto definition = LogicItemDefinition {
        .logicitem_type = LogicItemType::xor_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .orientation = orientation_t::right,
    };
    return add_logic_item(state, definition, position, insertion_mode);
}

TEST(EditableCircuitHandler, LogicItemCombineAddMoveDelete) {
    using namespace editable_circuit;
    using enum InsertionMode;
    auto layout = Layout {};
    auto setup = HandlerSetup {layout};

    auto id_0 = add_xor_element(setup.state, point_t {1, 1}, temporary);
    setup.validate();

    auto id_1 = add_xor_element(setup.state, point_t {10, 10}, insert_or_discard);
    setup.validate();

    move_or_delete_logic_item(layout, setup.sender, id_0, 10, 10);
    setup.validate();

    change_logic_item_insertion_mode(setup.state, id_0, collisions);
    ASSERT_EQ(layout.logic_items().display_state(id_0), display_state_t::colliding);
    setup.validate();

    change_logic_item_insertion_mode(setup.state, id_0, insert_or_discard);
    ASSERT_EQ(id_0, null_logicitem_id);
    setup.validate();

    id_1 = logicitem_id_t {0};
    change_logic_item_insertion_mode(setup.state, id_1, temporary);
    setup.validate();

    swap_and_delete_logic_item(layout, setup.sender, id_1);
    ASSERT_EQ(id_1, null_logicitem_id);
    setup.validate();

    // layout
    assert_logicitem_count(layout, 0);
}

}  // namespace logicsim