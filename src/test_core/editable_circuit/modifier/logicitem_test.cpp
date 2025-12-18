#include "test_core/editable_circuit/modifier/test_helpers.h"

#include "core/component/editable_circuit/editing/edit_logicitem.h"
#include "core/component/editable_circuit/modifier.h"
#include "core/format/container.h"
#include "core/layout.h"
#include "core/layout_message.h"
#include "core/logging.h"
#include "core/vocabulary/logicitem_definition.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

namespace editable_circuit {

//
// swap_and_delete_single_element
//

TEST(EditableCircuitModifierLogicItem, DeleteTemporaryElement) {
    using namespace info_message;
    using enum display_state_t;

    auto layout = Layout {};
    auto logicitem_id = add_and_element(layout, temporary);

    ASSERT_EQ(logicitem_id, logicitem_id_t {0});

    auto modifier = get_logging_modifier(layout);
    modifier.delete_temporary_logicitem(logicitem_id);
    Expects(is_valid(modifier));

    // logicitem_ids
    ASSERT_EQ(logicitem_id, null_logicitem_id);

    // layout
    ASSERT_EQ(modifier.circuit_data().layout.empty(), true);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 1);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(0),
              Message {LogicItemDeleted {logicitem_id_t {0}}});
}

TEST(EditableCircuitModifierLogicItem, DeletePreserving1) {
    using namespace info_message;
    using enum display_state_t;

    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    auto logicitem_id_1 =
        add_and_element(layout, temporary, connection_count_t {3}, point_t {2, 2});

    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});
    ASSERT_EQ(logicitem_id_1, logicitem_id_t {1});

    auto modifier = get_logging_modifier(layout);
    modifier.delete_temporary_logicitem(logicitem_id_0);
    Expects(is_valid(modifier));

    // logicitem_ids
    ASSERT_EQ(logicitem_id_0, null_logicitem_id);

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
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 2);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(0), message0);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(1), message1);
}

TEST(EditableCircuitModifierLogicItem, DeletePreserving2) {
    using namespace info_message;
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

    auto modifier = get_logging_modifier(layout);
    modifier.delete_temporary_logicitem(logicitem_id_1);
    Expects(is_valid(modifier));

    //  logicitem_ids
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
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 3);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(0), message0);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(1), message1);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(2), message2);
}

//
// is_logicitem_position_representable
//

TEST(EditableCircuitModifierLogicItem, IsRepresentableAndElement) {
    using namespace editing;
    ;

    using enum display_state_t;
    auto layout = Layout {};

    auto item_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {0, 0});

    constexpr static auto overflow = int {grid_t::max()} + 100;

    // true
    {
        const auto delta = move_delta_t {10, 10};
        ASSERT_EQ(is_logicitem_position_representable(layout, item_id_0, delta), true);
    }
    {
        const auto delta = move_delta_t {-10, -10};
        ASSERT_EQ(is_logicitem_position_representable(layout, item_id_0, delta), true);
    }

    // false
    {
        const auto delta = move_delta_t {overflow, 10};
        ASSERT_EQ(is_logicitem_position_representable(layout, item_id_0, delta), false);
    }
    {
        const auto delta = move_delta_t {-overflow, 10};
        ASSERT_EQ(is_logicitem_position_representable(layout, item_id_0, delta), false);
    }
    {
        const auto delta = move_delta_t {0, overflow};
        ASSERT_EQ(is_logicitem_position_representable(layout, item_id_0, delta), false);
    }
    {
        const auto delta = move_delta_t {0, -overflow};
        ASSERT_EQ(is_logicitem_position_representable(layout, item_id_0, delta), false);
    }
}

//
// move_or_delete_logicitem
//

TEST(EditableCircuitModifierLogicItem, MoveLogicItemSuccess) {
    using namespace info_message;

    using enum display_state_t;

    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto modifier = get_logging_modifier(layout);
    modifier.move_or_delete_temporary_logicitem(logicitem_id_0, move_delta_t {9, -11});
    Expects(is_valid(modifier));

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(modifier, 1);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {2},
                           point_t {10, -10});

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 0);
}

TEST(EditableCircuitModifierLogicItem, MoveLogicItemUnchecked) {
    using namespace info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto modifier = get_logging_modifier(layout);
    modifier.move_temporary_logicitem_unchecked(logicitem_id_0, move_delta_t {9, -11});
    Expects(is_valid(modifier));

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(modifier, 1);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {2},
                           point_t {10, -10});

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 0);
}

TEST(EditableCircuitModifierLogicItem, MoveLogicItemDeleted) {
    using namespace info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto modifier = get_logging_modifier(layout);
    constexpr static auto overflow = int {grid_t::max()} + 100;
    modifier.move_or_delete_temporary_logicitem(logicitem_id_0,
                                                move_delta_t {overflow, 0});
    Expects(is_valid(modifier));

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, null_logicitem_id);

    // layout
    assert_logicitem_count(modifier, 0);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 1);
    const auto m0 = Message {LogicItemDeleted {logicitem_id_t {0}}};
    ASSERT_EQ(modifier.circuit_data().messages.value().at(0), m0);
}

//
// change_logicitem_insertion_mode  forward
//

TEST(EditableCircuitModifierLogicItem, LogicItemChangeModeToTempValid) {
    using namespace info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto modifier = get_logging_modifier(layout);
    modifier.change_logicitem_insertion_mode(logicitem_id_0, InsertionMode::collisions);
    Expects(is_valid(modifier));

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(modifier, 1);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    ASSERT_EQ(get_display_state(modifier, logicitem_id_t {0}), valid);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 1);

    const auto m0 = Message {LogicItemInserted {
        .logicitem_id = logicitem_id_t {0},
        .data = to_layout_calculation_data(layout, logicitem_id_t {0}),
    }};
    ASSERT_EQ(modifier.circuit_data().messages.value().at(0), m0);
}

TEST(EditableCircuitModifierLogicItem, LogicItemChangeModeToInsert) {
    using namespace info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, temporary, connection_count_t {2}, point_t {1, 1});
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto modifier = get_logging_modifier(layout);
    modifier.change_logicitem_insertion_mode(logicitem_id_0,
                                             InsertionMode::insert_or_discard);
    Expects(is_valid(modifier));

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(modifier, 1);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    ASSERT_EQ(get_display_state(modifier, logicitem_id_t {0}), normal);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 1);
    const auto m0 = Message {
        LogicItemInserted {.logicitem_id = logicitem_id_t {0},
                           .data = to_layout_calculation_data(
                               modifier.circuit_data().layout, logicitem_id_t {0})}};
    ASSERT_EQ(modifier.circuit_data().messages.value().at(0), m0);
}

TEST(EditableCircuitModifierLogicItem, LogicItemChangeModeToTempColliding) {
    using namespace info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, normal, connection_count_t {2}, point_t {1, 1});
    auto logicitem_id_1 =
        add_and_element(layout, temporary, connection_count_t {3}, point_t {2, 2});
    assert_logicitem_count(layout, 2);
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});
    ASSERT_EQ(logicitem_id_1, logicitem_id_t {1});

    auto modifier = get_logging_modifier(layout);
    modifier.change_logicitem_insertion_mode(logicitem_id_1, InsertionMode::collisions);
    Expects(is_valid(modifier));

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
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 0);
}

TEST(EditableCircuitModifierLogicItem, LogicItemChangeModeToDiscard) {
    using namespace info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, normal, connection_count_t {2}, point_t {1, 1});
    auto logicitem_id_1 =
        add_and_element(layout, temporary, connection_count_t {3}, point_t {2, 2});
    assert_logicitem_count(layout, 2);
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});
    ASSERT_EQ(logicitem_id_1, logicitem_id_t {1});

    auto modifier = get_logging_modifier(layout);
    modifier.change_logicitem_insertion_mode(logicitem_id_1,
                                             InsertionMode::insert_or_discard);
    Expects(is_valid(modifier));

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_1, null_logicitem_id);

    // layout
    assert_logicitem_count(modifier, 1);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    ASSERT_EQ(get_display_state(modifier, logicitem_id_t {0}), normal);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 1);
    const auto message0 = Message {LogicItemDeleted {logicitem_id_t {1}}};
    ASSERT_EQ(modifier.circuit_data().messages.value().at(0), message0);
}

//
// change_logicitem_insertion_mode  backwards
//

TEST(EditableCircuitModifierLogicItem, LogicItemChangeModeBToValid) {
    using namespace info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, normal, connection_count_t {2}, point_t {1, 1});
    assert_logicitem_count(layout, 1);
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto modifier = get_logging_modifier(layout);
    modifier.change_logicitem_insertion_mode(logicitem_id_0, InsertionMode::collisions);
    Expects(is_valid(modifier));

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(modifier, 1);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    ASSERT_EQ(get_display_state(modifier, logicitem_id_t {0}), valid);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 0);
}

TEST(EditableCircuitModifierLogicItem, LogicItemChangeModeBToTemporary) {
    using namespace info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, normal, connection_count_t {2}, point_t {1, 1});
    assert_logicitem_count(layout, 1);
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    auto modifier = get_logging_modifier(layout);
    modifier.change_logicitem_insertion_mode(logicitem_id_0, InsertionMode::temporary);
    Expects(is_valid(modifier));

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(modifier, 1);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    ASSERT_EQ(get_display_state(modifier, logicitem_id_t {0}), temporary);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 1);
    const auto m0 = Message {
        LogicItemUninserted {.logicitem_id = logicitem_id_t {0},
                             .data = to_layout_calculation_data(
                                 modifier.circuit_data().layout, logicitem_id_t {0})}};
    ASSERT_EQ(modifier.circuit_data().messages.value().at(0), m0);
}

TEST(EditableCircuitModifierLogicItem, LogicItemChangeModeBToTemporaryPreserving) {
    using namespace info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto logicitem_id_0 =
        add_and_element(layout, normal, connection_count_t {2}, point_t {1, 1});

    assert_logicitem_count(layout, 1);
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});
    const auto data0 = to_layout_calculation_data(layout, logicitem_id_t {0});

    auto modifier = get_logging_modifier(layout);
    modifier.change_logicitem_insertion_mode(logicitem_id_0, InsertionMode::temporary);
    Expects(is_valid(modifier));

    //  logicitem_ids
    ASSERT_EQ(logicitem_id_0, logicitem_id_t {0});

    // layout
    assert_logicitem_count(modifier, 1);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {2},
                           point_t {1, 1});
    ASSERT_EQ(get_display_state(modifier, logicitem_id_t {0}), temporary);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 1);

    const auto m0 = Message {LogicItemUninserted {
        .logicitem_id = logicitem_id_t {0},
        .data = data0,
    }};
    ASSERT_EQ(modifier.circuit_data().messages.value().at(0), m0);
}

//
// add_standard_logicitem
//

TEST(EditableCircuitModifierLogicItem, LogicItemAddElement) {
    using namespace info_message;
    using enum display_state_t;
    auto layout = Layout {};

    auto definition = LogicItemDefinition {
        .logicitem_type = LogicItemType::xor_element,
        .input_count = connection_count_t {7},
        .output_count = connection_count_t {1},
        .orientation = orientation_t::right,
    };

    auto modifier = get_logging_modifier(layout);
    const auto logicitem_id = modifier.add_logicitem(
        std::move(definition), point_t {2, 3}, InsertionMode::insert_or_discard);
    Expects(is_valid(modifier));

    //  logicitem_ids
    ASSERT_EQ(logicitem_id, logicitem_id_t {0});

    // layout
    assert_logicitem_count(modifier, 1);
    assert_logicitem_equal(modifier, logicitem_id_t {0}, connection_count_t {7},
                           point_t {2, 3});
    ASSERT_EQ(get_display_state(modifier, logicitem_id_t {0}), normal);

    // messages
    ASSERT_EQ(modifier.circuit_data().messages.value().size(), 2);
    const auto m0 = Message {LogicItemCreated {logicitem_id_t {0}}};
    const auto m1 = Message {
        LogicItemInserted {.logicitem_id = logicitem_id_t {0},
                           .data = to_layout_calculation_data(
                               modifier.circuit_data().layout, logicitem_id_t {0})}};
    ASSERT_EQ(modifier.circuit_data().messages.value().at(0), m0);
    ASSERT_EQ(modifier.circuit_data().messages.value().at(1), m1);
}

//
// logic item combinations
//

auto add_xor_element(Modifier &modifier, point_t position, InsertionMode insertion_mode)
    -> logicitem_id_t {
    auto definition = LogicItemDefinition {
        .logicitem_type = LogicItemType::xor_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .orientation = orientation_t::right,
    };
    const auto id =
        modifier.add_logicitem(std::move(definition), position, insertion_mode);
    Expects(is_valid(modifier));
    return id;
}

TEST(EditableCircuitModifierLogicItem, LogicItemCombineAddMoveDelete) {
    using enum InsertionMode;
    auto layout = Layout {};
    auto modifier = get_logging_modifier(layout);

    auto id_0 = add_xor_element(modifier, point_t {1, 1}, temporary);
    auto id_1 = add_xor_element(modifier, point_t {10, 10}, insert_or_discard);

    modifier.move_or_delete_temporary_logicitem(id_0, move_delta_t {10, 10});
    Expects(is_valid(modifier));

    modifier.change_logicitem_insertion_mode(id_0, collisions);
    Expects(is_valid(modifier));
    ASSERT_EQ(get_display_state(modifier, id_0), display_state_t::colliding);

    modifier.change_logicitem_insertion_mode(id_0, insert_or_discard);
    Expects(is_valid(modifier));
    ASSERT_EQ(id_0, null_logicitem_id);

    id_1 = logicitem_id_t {0};
    modifier.change_logicitem_insertion_mode(id_1, temporary);
    Expects(is_valid(modifier));

    modifier.delete_temporary_logicitem(id_1);
    Expects(is_valid(modifier));
    ASSERT_EQ(id_1, null_logicitem_id);

    // layout
    assert_logicitem_count(modifier, 0);
}

}  // namespace editable_circuit

}  // namespace logicsim
