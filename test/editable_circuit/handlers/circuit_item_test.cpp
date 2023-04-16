
#include "./test_helpers.h"
#include "editable_circuit/handlers.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

//
// Test Setup
//

TEST(EditableCircuitHandler, VerificationSetup) {
    auto circuit = empty_circuit();
    auto setup = HandlerSetup {circuit};

    setup.validate();
    ASSERT_EQ(setup.state.layout.empty(), true);
    ASSERT_EQ(setup.state.schematic.empty(), true);
    ASSERT_EQ(setup.recorder.messages().empty(), true);
}

//
// swap_and_delete_single_element
//

TEST(EditableCircuitHandler, DeleteTemporaryElement) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();
    auto element_id = add_and_element(circuit, temporary);

    ASSERT_EQ(element_id, element_id_t {0});

    auto setup = HandlerSetup {circuit};
    auto preserved_id = element_id_t {0};
    swap_and_delete_single_element(circuit, setup.sender, element_id, &preserved_id);

    setup.validate();

    // element_ids
    ASSERT_EQ(element_id, null_element);
    ASSERT_EQ(preserved_id, null_element);

    // circuit
    ASSERT_EQ(setup.state.layout.empty(), true);
    ASSERT_EQ(setup.state.schematic.empty(), true);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 1);
    ASSERT_EQ(setup.recorder.messages().at(0),
              Message {LogicItemDeleted {element_id_t {0}}});
}

TEST(EditableCircuitHandler, DeletePreserving1) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();

    auto element_id_0 = add_and_element(circuit, temporary, 2, point_t {1, 1});
    auto element_id_1 = add_and_element(circuit, temporary, 3, point_t {2, 2});

    ASSERT_EQ(element_id_0, element_id_t {0});
    ASSERT_EQ(element_id_1, element_id_t {1});

    auto setup = HandlerSetup {circuit};
    swap_and_delete_single_element(circuit, setup.sender, element_id_0, &element_id_1);

    setup.validate();
    // element_ids
    ASSERT_EQ(element_id_0, null_element);
    ASSERT_EQ(element_id_1, element_id_t {0});

    // circuit
    assert_element_count(circuit, 1);
    assert_element_equal(circuit, element_id_t {0}, 3, point_t {2, 2});

    // messages
    const auto message0 = Message {LogicItemDeleted {element_id_t {0}}};
    const auto message1 = Message {LogicItemIdUpdated {
        .new_element_id = element_id_t {0},
        .old_element_id = element_id_t {1},
    }};
    ASSERT_EQ(setup.recorder.messages().size(), 2);
    ASSERT_EQ(setup.recorder.messages().at(0), message0);
    ASSERT_EQ(setup.recorder.messages().at(1), message1);
}

TEST(EditableCircuitHandler, DeletePreserving2) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();

    auto element_id_0 = add_and_element(circuit, temporary, 2, point_t {1, 1});
    auto element_id_1 = add_and_element(circuit, temporary, 3, point_t {2, 2});
    auto placeholder = add_placeholder(circuit);
    auto element_id_3 = add_and_element(circuit, valid, 5, point_t {4, 4});
    placeholder.input(connection_id_t {0})
        .connect(circuit.schematic().element(element_id_3).output(connection_id_t {0}));

    ASSERT_EQ(element_id_0, element_id_t {0});
    ASSERT_EQ(element_id_1, element_id_t {1});
    ASSERT_EQ(placeholder.element_id(), element_id_t {2});
    ASSERT_EQ(element_id_3, element_id_t {3});

    auto setup = HandlerSetup {circuit};
    swap_and_delete_single_element(circuit, setup.sender, element_id_1, &element_id_0);

    setup.validate();
    //  element_ids
    ASSERT_EQ(element_id_0, element_id_t {0});
    ASSERT_EQ(element_id_1, null_element);

    // circuit
    assert_element_count(circuit, 3);
    assert_element_equal(circuit, element_id_t {0}, 2, point_t {1, 1});
    assert_element_equal(circuit, element_id_t {1}, 5, point_t {4, 4});
    assert_is_placeholder(circuit, element_id_t {2});

    // messages
    const auto message0 = Message {LogicItemDeleted {element_id_t {1}}};
    const auto message1 = Message {LogicItemIdUpdated {
        .new_element_id = element_id_t {1}, .old_element_id = element_id_t {3}}};
    const auto message2 = Message {InsertedLogicItemIdUpdated {
        .new_element_id = element_id_t {1},
        .old_element_id = element_id_t {3},
        .data = to_layout_calculation_data(circuit, element_id_t {1})}};
    ASSERT_EQ(setup.recorder.messages().size(), 3);
    ASSERT_EQ(setup.recorder.messages().at(0), message0);
    ASSERT_EQ(setup.recorder.messages().at(1), message1);
    ASSERT_EQ(setup.recorder.messages().at(2), message2);
}

//
// swap_and_delete_multiple_elements
//

TEST(EditableCircuitHandler, DeleteMultipleElements) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();
    auto element_id_0 = add_and_element(circuit, temporary, 2, point_t {1, 1});
    auto element_id_1 = add_and_element(circuit, temporary, 3, point_t {2, 2});
    auto element_id_2 = add_and_element(circuit, temporary, 4, point_t {3, 3});  //
    auto element_id_3 = add_and_element(circuit, temporary, 5, point_t {4, 4});

    ASSERT_EQ(element_id_0, element_id_t {0});
    ASSERT_EQ(element_id_1, element_id_t {1});
    ASSERT_EQ(element_id_2, element_id_t {2});
    ASSERT_EQ(element_id_3, element_id_t {3});

    auto setup = HandlerSetup {circuit};
    auto to_delete = {element_id_1, element_id_0, element_id_3};
    auto preserved_id = element_id_2;
    swap_and_delete_multiple_elements(circuit, setup.sender, to_delete, &preserved_id);

    setup.validate();

    // element_ids
    ASSERT_EQ(preserved_id, element_id_t {0});

    // circuit
    assert_element_count(circuit, 1);
    assert_element_equal(circuit, element_id_t {0}, 4, point_t {3, 3});

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 5);
    const auto m0 = Message {LogicItemDeleted {element_id_t {3}}};
    const auto m1 = Message {LogicItemDeleted {element_id_t {1}}};
    const auto m2 = Message {LogicItemIdUpdated {.new_element_id = element_id_t {1},
                                                 .old_element_id = element_id_t {2}}};
    const auto m3 = Message {LogicItemDeleted {element_id_t {0}}};
    const auto m4 = Message {LogicItemIdUpdated {.new_element_id = element_id_t {0},
                                                 .old_element_id = element_id_t {1}}};
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
    ASSERT_EQ(setup.recorder.messages().at(1), m1);
    ASSERT_EQ(setup.recorder.messages().at(2), m2);
    ASSERT_EQ(setup.recorder.messages().at(3), m3);
    ASSERT_EQ(setup.recorder.messages().at(4), m4);
}

//
// is_logic_item_position_representable
//

TEST(EditableCircuitHandler, IsRepresentableAndElement) {
    using namespace editable_circuit;
    using enum display_state_t;
    auto circuit = empty_circuit();

    auto element_id_0 = add_and_element(circuit, temporary, 2, point_t {0, 0});

    constexpr static auto overflow = int {grid_t::max()} + 100;

    // true
    ASSERT_EQ(is_logic_item_position_representable(circuit, element_id_0, 10, 10), true);
    ASSERT_EQ(is_logic_item_position_representable(circuit, element_id_0, -10, -10),
              true);

    // false
    ASSERT_EQ(is_logic_item_position_representable(circuit, element_id_0, overflow, 10),
              false);
    ASSERT_EQ(is_logic_item_position_representable(circuit, element_id_0, -overflow, 10),
              false);
    ASSERT_EQ(is_logic_item_position_representable(circuit, element_id_0, 0, overflow),
              false);
    ASSERT_EQ(is_logic_item_position_representable(circuit, element_id_0, 0, -overflow),
              false);
}

//
// move_or_delete_logic_item
//

TEST(EditableCircuitHandler, MoveLogicItemSuccess) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();

    auto element_id_0 = add_and_element(circuit, temporary, 2, point_t {1, 1});
    ASSERT_EQ(element_id_0, element_id_t {0});

    auto setup = HandlerSetup {circuit};
    move_or_delete_logic_item(circuit, setup.sender, element_id_0, 10, -10);

    setup.validate();
    //  element_ids
    ASSERT_EQ(element_id_0, element_id_t {0});

    // circuit
    assert_element_count(circuit, 1);
    assert_element_equal(circuit, element_id_t {0}, 2, point_t {10, -10});

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 0);
}

TEST(EditableCircuitHandler, MoveLogicItemDeleted) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();

    auto element_id_0 = add_and_element(circuit, temporary, 2, point_t {1, 1});
    ASSERT_EQ(element_id_0, element_id_t {0});

    auto setup = HandlerSetup {circuit};
    constexpr static auto overflow = int {grid_t::max()} + 100;
    move_or_delete_logic_item(circuit, setup.sender, element_id_0, overflow, 0);

    setup.validate();
    //  element_ids
    ASSERT_EQ(element_id_0, null_element);

    // circuit
    assert_element_count(circuit, 0);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 1);
    const auto m0 = Message {LogicItemDeleted {element_id_t {0}}};
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
}

//
// change_logic_item_insertion_mode  forward
//

TEST(EditableCircuitHandler, LogicItemChangeModeToTempValid) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();

    auto element_id_0 = add_and_element(circuit, temporary, 2, point_t {1, 1});
    ASSERT_EQ(element_id_0, element_id_t {0});

    auto setup = HandlerSetup {circuit};
    change_logic_item_insertion_mode(setup.state, element_id_0,
                                     InsertionMode::collisions);

    setup.validate();
    //  element_ids
    ASSERT_EQ(element_id_0, element_id_t {0});

    // circuit
    assert_element_count(circuit, 2);
    assert_element_equal(circuit, element_id_t {0}, 2, point_t {1, 1});
    assert_is_placeholder(circuit, element_id_t {1});
    ASSERT_EQ(circuit.layout().display_state(element_id_t {0}), valid);
    ASSERT_EQ(circuit.layout().display_state(element_id_t {1}), normal);

    // placeholder connected
    const auto id_0 = connection_id_t {0};
    const auto and_output = circuit.schematic().element(element_id_t {0}).output(id_0);
    const auto placeholder_in = circuit.schematic().element(element_id_t {1}).input(id_0);
    ASSERT_EQ(and_output.connected_input(), placeholder_in);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 1);

    const auto m0 = Message {LogicItemInserted {
        .element_id = element_id_t {0},
        .data = to_layout_calculation_data(circuit, element_id_t {0})}};
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
}

TEST(EditableCircuitHandler, LogicItemChangeModeToInsert) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();

    auto element_id_0 = add_and_element(circuit, temporary, 2, point_t {1, 1});
    ASSERT_EQ(element_id_0, element_id_t {0});

    auto setup = HandlerSetup {circuit};
    change_logic_item_insertion_mode(setup.state, element_id_0,
                                     InsertionMode::insert_or_discard);

    setup.validate();
    //  element_ids
    ASSERT_EQ(element_id_0, element_id_t {0});

    // circuit
    assert_element_count(circuit, 2);
    assert_element_equal(circuit, element_id_t {0}, 2, point_t {1, 1});
    assert_is_placeholder(circuit, element_id_t {1});
    ASSERT_EQ(circuit.layout().display_state(element_id_t {0}), normal);
    ASSERT_EQ(circuit.layout().display_state(element_id_t {1}), normal);

    // placeholder connected
    const auto id_0 = connection_id_t {0};
    const auto and_output = circuit.schematic().element(element_id_t {0}).output(id_0);
    const auto placeholder_in = circuit.schematic().element(element_id_t {1}).input(id_0);
    ASSERT_EQ(and_output.connected_input(), placeholder_in);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 1);
    const auto m0 = Message {LogicItemInserted {
        .element_id = element_id_t {0},
        .data = to_layout_calculation_data(circuit, element_id_t {0})}};
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
}

TEST(EditableCircuitHandler, LogicItemChangeModeToTempColliding) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();

    auto element_id_0 = add_and_element(circuit, normal, 2, point_t {1, 1});
    auto element_id_1 = add_and_element(circuit, temporary, 3, point_t {2, 2});
    add_placeholders(circuit, element_id_0);
    assert_element_count(circuit, 3);
    ASSERT_EQ(element_id_0, element_id_t {0});
    ASSERT_EQ(element_id_1, element_id_t {1});
    assert_is_placeholder(circuit, element_id_t {2});

    auto setup = HandlerSetup {circuit};
    change_logic_item_insertion_mode(setup.state, element_id_1,
                                     InsertionMode::collisions);

    setup.validate();
    //  element_ids
    ASSERT_EQ(element_id_1, element_id_t {1});

    // circuit
    assert_element_count(circuit, 3);
    assert_element_equal(circuit, element_id_t {0}, 2, point_t {1, 1});
    assert_element_equal(circuit, element_id_t {1}, 3, point_t {2, 2});
    assert_is_placeholder(circuit, element_id_t {2});
    ASSERT_EQ(circuit.layout().display_state(element_id_t {0}), normal);
    ASSERT_EQ(circuit.layout().display_state(element_id_t {1}), colliding);
    ASSERT_EQ(circuit.layout().display_state(element_id_t {2}), normal);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 0);
}

TEST(EditableCircuitHandler, LogicItemChangeModeToDiscard) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();

    auto element_id_0 = add_and_element(circuit, normal, 2, point_t {1, 1});
    auto element_id_1 = add_and_element(circuit, temporary, 3, point_t {2, 2});
    add_placeholders(circuit, element_id_0);
    assert_element_count(circuit, 3);
    ASSERT_EQ(element_id_0, element_id_t {0});
    ASSERT_EQ(element_id_1, element_id_t {1});
    assert_is_placeholder(circuit, element_id_t {2});

    auto setup = HandlerSetup {circuit};
    change_logic_item_insertion_mode(setup.state, element_id_1,
                                     InsertionMode::insert_or_discard);

    setup.validate();
    //  element_ids
    ASSERT_EQ(element_id_1, null_element);

    // circuit
    assert_element_count(circuit, 2);
    assert_element_equal(circuit, element_id_t {0}, 2, point_t {1, 1});
    assert_is_placeholder(circuit, element_id_t {1});
    ASSERT_EQ(circuit.layout().display_state(element_id_t {0}), normal);
    ASSERT_EQ(circuit.layout().display_state(element_id_t {1}), normal);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 1);
    const auto message0 = Message {LogicItemDeleted {element_id_t {1}}};
    ASSERT_EQ(setup.recorder.messages().at(0), message0);
}

//
// change_logic_item_insertion_mode  backwards
//

TEST(EditableCircuitHandler, LogicItemChangeModeBToValid) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();

    auto element_id_0 = add_and_element(circuit, normal, 2, point_t {1, 1});
    add_placeholders(circuit, element_id_0);
    assert_element_count(circuit, 2);
    ASSERT_EQ(element_id_0, element_id_t {0});
    assert_is_placeholder(circuit, element_id_t {1});

    auto setup = HandlerSetup {circuit};
    change_logic_item_insertion_mode(setup.state, element_id_0,
                                     InsertionMode::collisions);

    setup.validate();
    //  element_ids
    ASSERT_EQ(element_id_0, element_id_t {0});

    // circuit
    assert_element_count(circuit, 2);
    assert_element_equal(circuit, element_id_t {0}, 2, point_t {1, 1});
    assert_is_placeholder(circuit, element_id_t {1});
    ASSERT_EQ(circuit.layout().display_state(element_id_t {0}), valid);
    ASSERT_EQ(circuit.layout().display_state(element_id_t {1}), normal);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 0);
}

TEST(EditableCircuitHandler, LogicItemChangeModeBToTemporary) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();

    auto element_id_0 = add_and_element(circuit, normal, 2, point_t {1, 1});
    add_placeholders(circuit, element_id_0);
    assert_element_count(circuit, 2);
    ASSERT_EQ(element_id_0, element_id_t {0});
    assert_is_placeholder(circuit, element_id_t {1});

    auto setup = HandlerSetup {circuit};
    change_logic_item_insertion_mode(setup.state, element_id_0, InsertionMode::temporary);

    setup.validate();
    //  element_ids
    ASSERT_EQ(element_id_0, element_id_t {0});

    // circuit
    assert_element_count(circuit, 1);
    assert_element_equal(circuit, element_id_t {0}, 2, point_t {1, 1});
    ASSERT_EQ(circuit.layout().display_state(element_id_t {0}), temporary);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 1);
    const auto m0 = Message {LogicItemUninserted {
        .element_id = element_id_t {0},
        .data = to_layout_calculation_data(circuit, element_id_t {0})}};
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
}

TEST(EditableCircuitHandler, LogicItemChangeModeBToTemporaryPreserving) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();
    const auto id_0 = connection_id_t {0};

    auto placeholder = add_placeholder(circuit);
    auto element_id_1 = add_and_element(circuit, normal, 2, point_t {1, 1});
    placeholder.input(id_0).connect(
        circuit.schematic().element(element_id_1).output(id_0));

    assert_element_count(circuit, 2);
    assert_is_placeholder(circuit, element_id_t {0});
    ASSERT_EQ(element_id_1, element_id_t {1});
    const auto data0 = to_layout_calculation_data(circuit, element_id_t {1});

    auto setup = HandlerSetup {circuit};
    change_logic_item_insertion_mode(setup.state, element_id_1, InsertionMode::temporary);

    setup.validate();
    //  element_ids
    ASSERT_EQ(element_id_1, element_id_t {0});

    // circuit
    assert_element_count(circuit, 1);
    assert_element_equal(circuit, element_id_t {0}, 2, point_t {1, 1});
    ASSERT_EQ(circuit.layout().display_state(element_id_t {0}), temporary);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 2);

    const auto m0
        = Message {LogicItemUninserted {.element_id = element_id_t {1}, .data = data0}};
    const auto m1 = Message {LogicItemIdUpdated {.new_element_id = element_id_t {0},
                                                 .old_element_id = element_id_t {1}}};
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
    ASSERT_EQ(setup.recorder.messages().at(1), m1);
}

//
// add_standard_logic_item
//

TEST(EditableCircuitHandler, LogicItemAddElement) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();

    auto setup = HandlerSetup {circuit};

    const auto attributes = editable_circuit::StandardLogicAttributes {
        .type = ElementType::xor_element,
        .input_count = 7,
        .position = point_t {2, 3},
        .orientation = orientation_t::right,
    };
    const auto element_id = add_standard_logic_item(setup.state, attributes,
                                                    InsertionMode::insert_or_discard);

    setup.validate();
    //  element_ids
    ASSERT_EQ(element_id, element_id_t {0});

    // circuit
    assert_element_count(circuit, 2);
    assert_element_equal(circuit, element_id_t {0}, 7, point_t {2, 3});
    assert_is_placeholder(circuit, element_id_t {1});
    ASSERT_EQ(circuit.layout().display_state(element_id_t {0}), normal);
    ASSERT_EQ(circuit.layout().display_state(element_id_t {1}), normal);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 2);
    const auto m0 = Message {LogicItemCreated {element_id_t {0}}};
    const auto m1 = Message {LogicItemInserted {
        .element_id = element_id_t {0},
        .data = to_layout_calculation_data(circuit, element_id_t {0})}};
    ASSERT_EQ(setup.recorder.messages().at(0), m0);
    ASSERT_EQ(setup.recorder.messages().at(1), m1);
}

//
// logic item combinations
//

auto add_xor_element(editable_circuit::State &state, point_t position,
                     InsertionMode insertion_mode) -> element_id_t {
    const auto attributes = editable_circuit::StandardLogicAttributes {
        .type = ElementType::xor_element,
        .input_count = 3,
        .position = position,
        .orientation = orientation_t::right,
    };
    return add_standard_logic_item(state, attributes, insertion_mode);
}

TEST(EditableCircuitHandler, LogicItemCombineAddMoveDelete) {
    using namespace editable_circuit;
    using enum InsertionMode;
    auto circuit = empty_circuit();
    auto setup = HandlerSetup {circuit};

    auto id_0 = add_xor_element(setup.state, point_t {1, 1}, temporary);
    setup.validate();

    auto id_1 = add_xor_element(setup.state, point_t {10, 10}, insert_or_discard);
    setup.validate();

    move_or_delete_logic_item(circuit, setup.sender, id_0, 10, 10);
    setup.validate();

    change_logic_item_insertion_mode(setup.state, id_0, collisions);
    ASSERT_EQ(circuit.layout().display_state(id_0), display_state_t::colliding);
    setup.validate();

    change_logic_item_insertion_mode(setup.state, id_0, insert_or_discard);
    ASSERT_EQ(id_0, null_element);
    setup.validate();

    change_logic_item_insertion_mode(setup.state, id_1, temporary);
    setup.validate();

    swap_and_delete_single_element(circuit, setup.sender, id_1);
    ASSERT_EQ(id_1, null_element);
    setup.validate();

    // circuit
    assert_element_count(circuit, 0);
}

}  // namespace logicsim