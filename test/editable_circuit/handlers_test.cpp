
#include "editable_circuit/handlers.h"

#include "editable_circuit/caches.h"
#include "format.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

namespace logicsim {

auto empty_circuit() -> Circuit {
    return Circuit {Schematic {}, Layout {}};
}

// forward message to cache and store them
class MessageRecorder : public editable_circuit::VirtualReceiver {
   public:
    MessageRecorder(CacheProvider &cache_provider) : cache_provider_ {cache_provider} {}

    auto submit(editable_circuit::InfoMessage message) -> void override {
        messages_.push_back(message);
        cache_provider_.submit(message);
    }

    auto messages() -> const std::vector<editable_circuit::InfoMessage> & {
        return messages_;
    }

   private:
    CacheProvider &cache_provider_;
    std::vector<editable_circuit::InfoMessage> messages_ {};
};

struct HandlerSetup {
    Circuit &circuit;
    CacheProvider cache;
    MessageRecorder recorder;
    editable_circuit::MessageSender sender;
    editable_circuit::State state;

    HandlerSetup(Circuit &circuit)
        : circuit {circuit},
          cache {circuit},
          recorder {cache},
          sender {editable_circuit::MessageSender {recorder}},
          state {circuit, sender, cache, circuit.schematic(), circuit.layout()} {
        validate();
    }

    auto validate() -> void {
        circuit.validate();
        cache.validate(circuit);
    }
};

auto add_and_element(Circuit &circuit, display_state_t display_type,
                     std::size_t input_count = 3, point_t position = point_t {0, 0})
    -> element_id_t {
    circuit.schematic().add_element(Schematic::NewElementData {
        .element_type = ElementType::and_element,
        .input_count = input_count,
        .output_count = 1,
    });
    return circuit.layout().add_logic_element(position, orientation_t::right,
                                              display_type);
}

auto add_placeholder(Circuit &circuit) -> Schematic::Element {
    const auto element = circuit.schematic().add_element(Schematic::NewElementData {
        .element_type = ElementType::placeholder,
        .input_count = 1,
        .output_count = 0,
    });
    circuit.layout().add_placeholder(display_state_t::normal);
    return element;
}

auto add_placeholder(Circuit &circuit, Schematic::Output output) -> element_id_t {
    const auto element = add_placeholder(circuit);
    element.input(connection_id_t {0}).connect(output);
    return element.element_id();
}

auto assert_element_count(const Circuit &circuit, std::size_t count) -> void {
    ASSERT_EQ(circuit.schematic().element_count(), count);
    ASSERT_EQ(circuit.layout().element_count(), count);
}

auto assert_element_equal(const Circuit &circuit, element_id_t element_id,
                          std::size_t input_count = 3, point_t position = point_t {0, 0})
    -> void {
    ASSERT_EQ(circuit.schematic().element(element_id).input_count(), input_count);
    ASSERT_EQ(circuit.layout().position(element_id), position);
}

auto assert_is_placeholder(const Circuit &circuit, element_id_t element_id) -> void {
    ASSERT_EQ(circuit.schematic().element(element_id).is_placeholder(), true);
    ASSERT_EQ(circuit.layout().display_state(element_id), display_state_t::normal);
}

//
// Setup
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
    auto element_id = add_and_element(circuit, new_temporary);

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
              Message {ElementDeleted {element_id_t {0}}});
}

TEST(EditableCircuitHandler, DeletePreserving1) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();

    auto element_id_0 = add_and_element(circuit, new_temporary, 2, point_t {1, 1});
    auto element_id_1 = add_and_element(circuit, new_temporary, 3, point_t {2, 2});

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
    const auto message0 = Message {ElementDeleted {element_id_t {0}}};
    const auto message1 = Message {ElementUpdated {element_id_t {0}, element_id_t {1}}};
    ASSERT_EQ(setup.recorder.messages().size(), 2);
    ASSERT_EQ(setup.recorder.messages().at(0), message0);
    ASSERT_EQ(setup.recorder.messages().at(1), message1);
}

TEST(EditableCircuitHandler, DeletePreserving2) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();

    auto element_id_0 = add_and_element(circuit, new_temporary, 2, point_t {1, 1});
    auto element_id_1 = add_and_element(circuit, new_temporary, 3, point_t {2, 2});
    auto placeholder = add_placeholder(circuit);
    auto element_id_3 = add_and_element(circuit, new_valid, 5, point_t {4, 4});
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
    auto data_1 = to_layout_calculation_data(circuit, element_id_t {1});
    const auto message0 = Message {ElementDeleted {element_id_t {1}}};
    const auto message1 = Message {ElementUpdated {element_id_t {1}, element_id_t {3}}};
    const auto message2
        = Message {InsertedLogicItemUpdated {element_id_t {1}, element_id_t {3}, data_1}};
    ASSERT_EQ(setup.recorder.messages().size(), 3);
    ASSERT_EQ(setup.recorder.messages().at(0), message0);
    ASSERT_EQ(setup.recorder.messages().at(1), message1);
    ASSERT_EQ(setup.recorder.messages().at(2), message2);
}

//
// is_logic_item_position_representable
//

TEST(EditableCircuitHandler, IsRepresentableAndElement) {
    using namespace editable_circuit;
    using enum display_state_t;
    auto circuit = empty_circuit();

    auto element_id_0 = add_and_element(circuit, new_temporary, 2, point_t {0, 0});

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

    auto element_id_0 = add_and_element(circuit, new_temporary, 2, point_t {1, 1});
    ASSERT_EQ(element_id_0, element_id_t {0});

    auto setup = HandlerSetup {circuit};
    move_or_delete_logic_item(setup.state, element_id_0, 10, -10);

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

    auto element_id_0 = add_and_element(circuit, new_temporary, 2, point_t {1, 1});
    ASSERT_EQ(element_id_0, element_id_t {0});

    auto setup = HandlerSetup {circuit};
    constexpr static auto overflow = int {grid_t::max()} + 100;
    move_or_delete_logic_item(setup.state, element_id_0, overflow, 0);

    setup.validate();
    //  element_ids
    ASSERT_EQ(element_id_0, null_element);

    // circuit
    assert_element_count(circuit, 0);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 1);
    const auto message0 = Message {ElementDeleted {element_id_t {0}}};
    ASSERT_EQ(setup.recorder.messages().at(0), message0);
}

//
// change_logic_item_insertion_mode
//

TEST(EditableCircuitHandler, LogicItemChangeModeToCollision) {
    using namespace editable_circuit::info_message;
    using enum display_state_t;
    auto circuit = empty_circuit();

    auto element_id_0 = add_and_element(circuit, new_temporary, 2, point_t {1, 1});
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
    ASSERT_EQ(circuit.layout().display_state(element_id_t {0}), new_valid);
    ASSERT_EQ(circuit.layout().display_state(element_id_t {1}), normal);

    // placeholder connected
    const auto id_0 = connection_id_t {0};
    const auto and_output = circuit.schematic().element(element_id_t {0}).output(id_0);
    const auto placeholder_in = circuit.schematic().element(element_id_t {1}).input(id_0);
    ASSERT_EQ(and_output.connected_input(), placeholder_in);

    // messages
    ASSERT_EQ(setup.recorder.messages().size(), 1);
    auto data_0 = to_layout_calculation_data(circuit, element_id_t {0});
    const auto message0 = Message {LogicItemInserted {element_id_t {0}, data_0}};
    ASSERT_EQ(setup.recorder.messages().at(0), message0);
}

}  // namespace logicsim