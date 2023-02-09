
#include "circuit.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <ranges>

namespace logicsim {

TEST(Circuit, EmptyCircuit) {
    const Circuit circuit;

    EXPECT_EQ(circuit.element_count(), 0);
    EXPECT_EQ(circuit.input_count(), 0);
    EXPECT_EQ(circuit.output_count(), 0);
    EXPECT_EQ(std::ranges::distance(circuit.elements()), 0);

    circuit.validate();
}

TEST(Circuit, CircuitSingleElement) {
    Circuit circuit;

    circuit.add_element(ElementType::wire, connection_size_t {3}, connection_size_t {5});

    EXPECT_EQ(circuit.element_count(), 1);
    EXPECT_EQ(circuit.input_count(), 3);
    EXPECT_EQ(circuit.output_count(), 5);
    EXPECT_EQ(std::ranges::distance(circuit.elements()), 1);

    circuit.validate();
}

TEST(Circuit, ElementProperties) {
    Circuit circuit;
    circuit.add_element(ElementType::wire, connection_size_t {3}, connection_size_t {5});

    const Circuit& circuit_const {circuit};
    const Circuit::ConstElement element {circuit_const.element(element_id_t {0})};

    EXPECT_EQ(element.element_id(), element_id_t {0});
    EXPECT_EQ(element.element_type(), ElementType::wire);
    EXPECT_EQ(element.input_count(), connection_size_t {3});
    EXPECT_EQ(element.output_count(), connection_size_t {5});

    EXPECT_EQ(std::ranges::distance(element.inputs()), 3);
    EXPECT_EQ(std::ranges::distance(element.outputs()), 5);

    circuit.validate();
    circuit_const.validate();
}

TEST(Circuit, EqualityOperators) {
    Circuit circuit;

    auto wire {circuit.add_element(ElementType::wire, connection_size_t {3},
                                   connection_size_t {5})};
    auto inverter {circuit.add_element(ElementType::inverter_element,
                                       connection_size_t {3}, connection_size_t {2})};

    const Circuit& circuit_const {circuit};

    EXPECT_EQ(wire, wire);
    EXPECT_EQ(wire, circuit_const.element(element_id_t {0}));
    EXPECT_NE(wire, inverter);

    auto id_0 = connection_size_t {0};
    auto id_1 = connection_size_t {1};

    EXPECT_EQ(wire.output(id_0), wire.output(id_0));
    EXPECT_EQ(wire.output(id_0), circuit_const.element(element_id_t {0}).output(id_0));
    EXPECT_NE(wire.output(id_0), inverter.output(id_0));
    EXPECT_NE(wire.output(id_0), wire.output(id_1));
    EXPECT_NE(wire.output(id_0), circuit_const.element(element_id_t {0}).output(id_1));

    circuit_const.validate();
    circuit.validate();
}

TEST(Circuit, ConnectionProperties) {
    Circuit circuit;

    auto wire {circuit.add_element(ElementType::wire, connection_size_t {3},
                                   connection_size_t {5})};
    auto inverter {circuit.add_element(ElementType::inverter_element,
                                       connection_size_t {3}, connection_size_t {2})};

    auto id_1 = connection_size_t {1};

    EXPECT_EQ(wire.output(id_1).element_id(), wire.element_id());
    EXPECT_EQ(wire.output(id_1).output_index(), connection_size_t {1});
    EXPECT_EQ(wire.output(id_1).element(), wire);
    EXPECT_EQ(wire.output(id_1).has_connected_element(), false);

    EXPECT_EQ(inverter.input(id_1).element_id(), inverter.element_id());
    EXPECT_EQ(inverter.input(id_1).input_index(), connection_size_t {1});
    EXPECT_EQ(inverter.input(id_1).element(), inverter);
    EXPECT_EQ(inverter.input(id_1).has_connected_element(), false);

    circuit.validate();
}

TEST(Circuit, ConnectedOutput) {
    Circuit circuit;

    auto wire {circuit.add_element(ElementType::wire, connection_size_t {3},
                                   connection_size_t {5})};
    auto inverter {circuit.add_element(ElementType::inverter_element,
                                       connection_size_t {3}, connection_size_t {2})};

    auto id_1 = connection_size_t {1};
    wire.output(id_1).connect(inverter.input(id_1));

    EXPECT_EQ(wire.output(id_1).has_connected_element(), true);
    EXPECT_EQ(wire.output(id_1).connected_element_id(), inverter.element_id());
    EXPECT_EQ(wire.output(id_1).connected_element(), inverter);
    EXPECT_EQ(wire.output(id_1).connected_input(), inverter.input(id_1));

    EXPECT_EQ(inverter.input(id_1).has_connected_element(), true);
    EXPECT_EQ(inverter.input(id_1).connected_element_id(), wire.element_id());
    EXPECT_EQ(inverter.input(id_1).connected_element(), wire);
    EXPECT_EQ(inverter.input(id_1).connected_output(), wire.output(id_1));

    circuit.validate();
}

TEST(Circuit, ConnectInput) {
    Circuit circuit;

    auto wire {circuit.add_element(ElementType::wire, connection_size_t {3},
                                   connection_size_t {5})};
    auto inverter {circuit.add_element(ElementType::inverter_element,
                                       connection_size_t {3}, connection_size_t {2})};

    auto id_1 = connection_size_t {1};
    inverter.input(id_1).connect(wire.output(id_1));

    EXPECT_EQ(wire.output(id_1).has_connected_element(), true);
    EXPECT_EQ(wire.output(id_1).connected_element_id(), inverter.element_id());
    EXPECT_EQ(wire.output(id_1).connected_element(), inverter);
    EXPECT_EQ(wire.output(id_1).connected_input(), inverter.input(id_1));

    EXPECT_EQ(inverter.input(id_1).has_connected_element(), true);
    EXPECT_EQ(inverter.input(id_1).connected_element_id(), wire.element_id());
    EXPECT_EQ(inverter.input(id_1).connected_element(), wire);
    EXPECT_EQ(inverter.input(id_1).connected_output(), wire.output(id_1));

    circuit.validate();
}

TEST(Circuit, ClearedInput) {
    Circuit circuit;

    auto wire {circuit.add_element(ElementType::wire, connection_size_t {3},
                                   connection_size_t {5})};
    auto inverter {circuit.add_element(ElementType::inverter_element,
                                       connection_size_t {3}, connection_size_t {2})};

    auto id_1 = connection_size_t {1};
    wire.output(id_1).connect(inverter.input(id_1));
    inverter.input(id_1).clear_connection();

    EXPECT_EQ(inverter.input(id_1).has_connected_element(), false);
    EXPECT_EQ(wire.output(id_1).has_connected_element(), false);

    circuit.validate();
}

TEST(Circuit, ClearedOutput) {
    Circuit circuit;

    auto wire {circuit.add_element(ElementType::wire, connection_size_t {3},
                                   connection_size_t {5})};
    auto inverter {circuit.add_element(ElementType::inverter_element,
                                       connection_size_t {3}, connection_size_t {2})};

    auto id_1 = connection_size_t {1};
    wire.output(id_1).connect(inverter.input(id_1));
    wire.output(id_1).clear_connection();

    EXPECT_EQ(inverter.input(id_1).has_connected_element(), false);
    EXPECT_EQ(wire.output(id_1).has_connected_element(), false);

    circuit.validate();
}

TEST(Circuit, ReconnectInput) {
    Circuit circuit;

    auto wire {circuit.add_element(ElementType::wire, connection_size_t {3},
                                   connection_size_t {5})};
    auto inverter {circuit.add_element(ElementType::inverter_element,
                                       connection_size_t {3}, connection_size_t {2})};

    auto id_1 = connection_size_t {1};
    wire.output(id_1).connect(inverter.input(id_1));
    inverter.input(id_1).connect(inverter.output(id_1));

    EXPECT_EQ(wire.output(id_1).has_connected_element(), false);
    EXPECT_EQ(inverter.input(id_1).has_connected_element(), true);
    EXPECT_EQ(inverter.output(id_1).has_connected_element(), true);

    circuit.validate();
}

TEST(Circuit, ReconnectOutput) {
    Circuit circuit;

    auto wire {circuit.add_element(ElementType::wire, connection_size_t {3},
                                   connection_size_t {5})};
    auto inverter {circuit.add_element(ElementType::inverter_element,
                                       connection_size_t {3}, connection_size_t {2})};

    auto id_1 = connection_size_t {1};
    wire.output(id_1).connect(inverter.input(id_1));
    wire.output(id_1).connect(wire.input(id_1));

    EXPECT_EQ(wire.output(id_1).has_connected_element(), true);
    EXPECT_EQ(inverter.input(id_1).has_connected_element(), false);
    EXPECT_EQ(wire.input(id_1).has_connected_element(), true);

    circuit.validate();
}

TEST(Circuit, TestPlaceholders) {
    Circuit circuit;
    auto wire {circuit.add_element(ElementType::wire, connection_size_t {3},
                                   connection_size_t {5})};
    EXPECT_EQ(circuit.element_count(), 1);

    add_output_placeholders(circuit);
    EXPECT_EQ(circuit.element_count(), 6);

    EXPECT_EQ(wire.output(connection_size_t {3}).has_connected_element(), true);
    EXPECT_EQ(wire.output(connection_size_t {3}).connected_element().element_type(),
              ElementType::placeholder);

    circuit.validate();
    circuit.validate(true);
}

//
// Element View
//

TEST(Circuit, ElementViewEmpty) {
    Circuit circuit;

    auto view = Circuit::ElementView {circuit};

    ASSERT_THAT(view, testing::ElementsAre());
    ASSERT_EQ(std::empty(view), true);
    ASSERT_EQ(std::size(view), 0);
}

TEST(Circuit, ElementViewFull) {
    Circuit circuit;
    auto wire = circuit.add_element(ElementType::wire, connection_size_t {1},
                                    connection_size_t {1});
    auto inverter = circuit.add_element(ElementType::inverter_element,
                                        connection_size_t {1}, connection_size_t {1});

    auto view = Circuit::ElementView {circuit};

    ASSERT_THAT(view, testing::ElementsAre(wire, inverter));
    ASSERT_EQ(std::empty(view), false);
    ASSERT_EQ(std::size(view), 2);
}

TEST(Circuit, ElementViewRanges) {
    Circuit circuit;
    circuit.add_element(ElementType::wire, connection_size_t {1}, connection_size_t {1});
    circuit.add_element(ElementType::inverter_element, connection_size_t {1},
                        connection_size_t {1});

    auto view = Circuit::ElementView {circuit};

    ASSERT_EQ(std::distance(view.begin(), view.end()), 2);
    ASSERT_EQ(std::ranges::distance(std::ranges::begin(view), std::ranges::end(view)), 2);
    ASSERT_EQ(std::ranges::distance(view), 2);
}

TEST(Circuit, ElementViewRangesLEGACY) {  // TODO remove when not needed
    Circuit circuit;
    circuit.add_element(ElementType::wire, connection_size_t {1}, connection_size_t {1});
    circuit.add_element(ElementType::inverter_element, connection_size_t {1},
                        connection_size_t {1});

    auto view = Circuit::ElementView {circuit};

    ASSERT_EQ(std::ranges::distance(std::ranges::begin(view), std::ranges::end(view)), 2);
    ASSERT_EQ(std::ranges::distance(view), 2);
}

//
// Element Inputs View
//

TEST(Circuit, InputsViewEmpty) {
    Circuit circuit;
    auto wire = circuit.add_element(ElementType::wire, connection_size_t {0},
                                    connection_size_t {1});
    auto view = Circuit::InputView {wire};

    ASSERT_THAT(view, testing::ElementsAre());
    ASSERT_EQ(std::empty(view), true);
    ASSERT_EQ(std::size(view), connection_size_t {0});
}

TEST(Circuit, InputsViewFull) {
    Circuit circuit;
    auto wire = circuit.add_element(ElementType::wire, connection_size_t {2},
                                    connection_size_t {1});
    auto view = Circuit::InputView {wire};

    ASSERT_THAT(view, testing::ElementsAre(wire.input(connection_size_t {0}),
                                           wire.input(connection_size_t {1})));
    ASSERT_EQ(std::empty(view), false);
    ASSERT_EQ(std::size(view), connection_size_t {2});
}

TEST(Circuit, InputsViewRanges) {
    Circuit circuit;
    auto wire = circuit.add_element(ElementType::wire, connection_size_t {2},
                                    connection_size_t {1});
    auto view = Circuit::InputView {wire};

    ASSERT_EQ(std::distance(view.begin(), view.end()), 2);
    ASSERT_EQ(std::ranges::distance(std::ranges::begin(view), std::ranges::end(view)), 2);
    ASSERT_EQ(std::ranges::distance(view), 2);
}

TEST(Circuit, InputsViewRangesLEGACY) {  // TODO remove when not needed
    Circuit circuit;
    auto wire = circuit.add_element(ElementType::wire, connection_size_t {2},
                                    connection_size_t {1});
    auto view = Circuit::InputView {wire};

    ASSERT_EQ(std::ranges::distance(std::ranges::begin(view), std::ranges::end(view)), 2);
    ASSERT_EQ(std::ranges::distance(view), 2);
}

}  // namespace logicsim
