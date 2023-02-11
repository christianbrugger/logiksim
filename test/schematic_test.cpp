
#include "schematic.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <ranges>

namespace logicsim {

TEST(Schematic, EmptySchematic) {
    const Schematic schematic;

    EXPECT_EQ(schematic.element_count(), 0);
    EXPECT_EQ(schematic.input_count(), 0);
    EXPECT_EQ(schematic.output_count(), 0);
    EXPECT_EQ(std::ranges::distance(schematic.elements()), 0);

    schematic.validate();
}

TEST(Schematic, SchematicSingleElement) {
    Schematic schematic;

    schematic.add_element(ElementType::wire, 3, 5);

    EXPECT_EQ(schematic.element_count(), 1);
    EXPECT_EQ(schematic.input_count(), 3);
    EXPECT_EQ(schematic.output_count(), 5);
    EXPECT_EQ(std::ranges::distance(schematic.elements()), 1);

    schematic.validate();
}

TEST(Schematic, ElementProperties) {
    Schematic schematic;
    schematic.add_element(ElementType::wire, 3, 5);

    const Schematic& schematic_const {schematic};
    const Schematic::ConstElement element {schematic_const.element(element_id_t {0})};

    EXPECT_EQ(element.element_id(), element_id_t {0});
    EXPECT_EQ(element.element_type(), ElementType::wire);
    EXPECT_EQ(element.input_count(), 3);
    EXPECT_EQ(element.output_count(), 5);

    EXPECT_EQ(std::ranges::distance(element.inputs()), 3);
    EXPECT_EQ(std::ranges::distance(element.outputs()), 5);

    schematic.validate();
    schematic_const.validate();
}

TEST(Schematic, EqualityOperators) {
    Schematic schematic;

    auto wire {schematic.add_element(ElementType::wire, 3, 5)};
    auto inverter {schematic.add_element(ElementType::inverter_element, 3, 2)};

    const Schematic& schematic_const {schematic};

    EXPECT_EQ(wire, wire);
    EXPECT_EQ(wire, schematic_const.element(element_id_t {0}));
    EXPECT_NE(wire, inverter);

    auto id_0 = connection_id_t {0};
    auto id_1 = connection_id_t {1};

    EXPECT_EQ(wire.output(id_0), wire.output(id_0));
    EXPECT_EQ(wire.output(id_0), schematic_const.element(element_id_t {0}).output(id_0));
    EXPECT_NE(wire.output(id_0), inverter.output(id_0));
    EXPECT_NE(wire.output(id_0), wire.output(id_1));
    EXPECT_NE(wire.output(id_0), schematic_const.element(element_id_t {0}).output(id_1));

    schematic_const.validate();
    schematic.validate();
}

TEST(Schematic, ConnectionProperties) {
    Schematic schematic;

    auto wire {schematic.add_element(ElementType::wire, 3, 5)};
    auto inverter {schematic.add_element(ElementType::inverter_element, 3, 2)};

    auto id_1 = connection_id_t {1};

    EXPECT_EQ(wire.output(id_1).element_id(), wire.element_id());
    EXPECT_EQ(wire.output(id_1).output_index(), connection_id_t {1});
    EXPECT_EQ(wire.output(id_1).element(), wire);
    EXPECT_EQ(wire.output(id_1).has_connected_element(), false);

    EXPECT_EQ(inverter.input(id_1).element_id(), inverter.element_id());
    EXPECT_EQ(inverter.input(id_1).input_index(), connection_id_t {1});
    EXPECT_EQ(inverter.input(id_1).element(), inverter);
    EXPECT_EQ(inverter.input(id_1).has_connected_element(), false);

    schematic.validate();
}

TEST(Schematic, ConnectedOutput) {
    Schematic schematic;

    auto wire {schematic.add_element(ElementType::wire, 3, 5)};
    auto inverter {schematic.add_element(ElementType::inverter_element, 3, 2)};

    auto id_1 = connection_id_t {1};
    wire.output(id_1).connect(inverter.input(id_1));

    EXPECT_EQ(wire.output(id_1).has_connected_element(), true);
    EXPECT_EQ(wire.output(id_1).connected_element_id(), inverter.element_id());
    EXPECT_EQ(wire.output(id_1).connected_element(), inverter);
    EXPECT_EQ(wire.output(id_1).connected_input(), inverter.input(id_1));

    EXPECT_EQ(inverter.input(id_1).has_connected_element(), true);
    EXPECT_EQ(inverter.input(id_1).connected_element_id(), wire.element_id());
    EXPECT_EQ(inverter.input(id_1).connected_element(), wire);
    EXPECT_EQ(inverter.input(id_1).connected_output(), wire.output(id_1));

    schematic.validate();
}

TEST(Schematic, ConnectInput) {
    Schematic schematic;

    auto wire {schematic.add_element(ElementType::wire, 3, 5)};
    auto inverter {schematic.add_element(ElementType::inverter_element, 3, 2)};

    auto id_1 = connection_id_t {1};
    inverter.input(id_1).connect(wire.output(id_1));

    EXPECT_EQ(wire.output(id_1).has_connected_element(), true);
    EXPECT_EQ(wire.output(id_1).connected_element_id(), inverter.element_id());
    EXPECT_EQ(wire.output(id_1).connected_element(), inverter);
    EXPECT_EQ(wire.output(id_1).connected_input(), inverter.input(id_1));

    EXPECT_EQ(inverter.input(id_1).has_connected_element(), true);
    EXPECT_EQ(inverter.input(id_1).connected_element_id(), wire.element_id());
    EXPECT_EQ(inverter.input(id_1).connected_element(), wire);
    EXPECT_EQ(inverter.input(id_1).connected_output(), wire.output(id_1));

    schematic.validate();
}

TEST(Schematic, ClearedInput) {
    Schematic schematic;

    auto wire {schematic.add_element(ElementType::wire, 3, 5)};
    auto inverter {schematic.add_element(ElementType::inverter_element, 3, 2)};

    auto id_1 = connection_id_t {1};
    wire.output(id_1).connect(inverter.input(id_1));
    inverter.input(id_1).clear_connection();

    EXPECT_EQ(inverter.input(id_1).has_connected_element(), false);
    EXPECT_EQ(wire.output(id_1).has_connected_element(), false);

    schematic.validate();
}

TEST(Schematic, ClearedOutput) {
    Schematic schematic;

    auto wire {schematic.add_element(ElementType::wire, 3, 5)};
    auto inverter {schematic.add_element(ElementType::inverter_element, 3, 2)};

    auto id_1 = connection_id_t {1};
    wire.output(id_1).connect(inverter.input(id_1));
    wire.output(id_1).clear_connection();

    EXPECT_EQ(inverter.input(id_1).has_connected_element(), false);
    EXPECT_EQ(wire.output(id_1).has_connected_element(), false);

    schematic.validate();
}

TEST(Schematic, ReconnectInput) {
    Schematic schematic;

    auto wire {schematic.add_element(ElementType::wire, 3, 5)};
    auto inverter {schematic.add_element(ElementType::inverter_element, 3, 2)};

    auto id_1 = connection_id_t {1};
    wire.output(id_1).connect(inverter.input(id_1));
    inverter.input(id_1).connect(inverter.output(id_1));

    EXPECT_EQ(wire.output(id_1).has_connected_element(), false);
    EXPECT_EQ(inverter.input(id_1).has_connected_element(), true);
    EXPECT_EQ(inverter.output(id_1).has_connected_element(), true);

    schematic.validate();
}

TEST(Schematic, ReconnectOutput) {
    Schematic schematic;

    auto wire {schematic.add_element(ElementType::wire, 3, 5)};
    auto inverter {schematic.add_element(ElementType::inverter_element, 3, 2)};

    auto id_1 = connection_id_t {1};
    wire.output(id_1).connect(inverter.input(id_1));
    wire.output(id_1).connect(wire.input(id_1));

    EXPECT_EQ(wire.output(id_1).has_connected_element(), true);
    EXPECT_EQ(inverter.input(id_1).has_connected_element(), false);
    EXPECT_EQ(wire.input(id_1).has_connected_element(), true);

    schematic.validate();
}

TEST(Schematic, TestPlaceholders) {
    Schematic schematic;
    auto wire {schematic.add_element(ElementType::wire, 3, 5)};
    EXPECT_EQ(schematic.element_count(), 1);

    add_output_placeholders(schematic);
    EXPECT_EQ(schematic.element_count(), 6);

    EXPECT_EQ(wire.output(connection_id_t {3}).has_connected_element(), true);
    EXPECT_EQ(wire.output(connection_id_t {3}).connected_element().element_type(),
              ElementType::placeholder);

    schematic.validate();
    schematic.validate(true);
}

//
// Element View
//

TEST(Schematic, ElementViewEmpty) {
    Schematic schematic;

    auto view = Schematic::ElementView {schematic};

    ASSERT_THAT(view, testing::ElementsAre());
    ASSERT_EQ(std::empty(view), true);
    ASSERT_EQ(std::size(view), 0);
}

TEST(Schematic, ElementViewFull) {
    Schematic schematic;
    auto wire {schematic.add_element(ElementType::wire, 1, 1)};
    auto inverter {schematic.add_element(ElementType::inverter_element, 1, 1)};

    auto view = Schematic::ElementView {schematic};

    ASSERT_THAT(view, testing::ElementsAre(wire, inverter));
    ASSERT_EQ(std::empty(view), false);
    ASSERT_EQ(std::size(view), 2);
}

TEST(Schematic, ElementViewRanges) {
    Schematic schematic;
    schematic.add_element(ElementType::wire, 1, 1);
    schematic.add_element(ElementType::inverter_element, 1, 1);

    auto view = Schematic::ElementView {schematic};

    ASSERT_EQ(std::distance(view.begin(), view.end()), 2);
    ASSERT_EQ(std::ranges::distance(std::ranges::begin(view), std::ranges::end(view)), 2);
    ASSERT_EQ(std::ranges::distance(view), 2);
}

//
// Element Inputs View
//

TEST(Schematic, InputsViewEmpty) {
    Schematic schematic;
    auto wire = schematic.add_element(ElementType::wire, 0, 1);
    auto view = Schematic::InputView {wire};

    ASSERT_THAT(view, testing::ElementsAre());
    ASSERT_EQ(std::empty(view), true);
    ASSERT_EQ(std::size(view), 0);
}

TEST(Schematic, InputsViewFull) {
    Schematic schematic;
    auto wire = schematic.add_element(ElementType::wire, 2, 1);
    auto view = Schematic::InputView {wire};

    ASSERT_THAT(view, testing::ElementsAre(wire.input(connection_id_t {0}),
                                           wire.input(connection_id_t {1})));
    ASSERT_EQ(std::empty(view), false);
    ASSERT_EQ(std::size(view), 2);
}

TEST(Schematic, InputsViewRanges) {
    Schematic schematic;
    auto wire = schematic.add_element(ElementType::wire, 2, 1);
    auto view = Schematic::InputView {wire};

    ASSERT_EQ(std::distance(view.begin(), view.end()), 2);
    ASSERT_EQ(std::ranges::distance(std::ranges::begin(view), std::ranges::end(view)), 2);
    ASSERT_EQ(std::ranges::distance(view), 2);
}

}  // namespace logicsim
