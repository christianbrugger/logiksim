
#include "logic_item/schematic_info.h"
#include "schematic_old.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <ranges>

namespace logicsim {

TEST(SchematicOld, EmptySchematic) {
    const SchematicOld schematic;

    EXPECT_EQ(schematic.element_count(), 0);
    EXPECT_EQ(schematic.total_input_count(), 0);
    EXPECT_EQ(schematic.total_output_count(), 0);
    EXPECT_EQ(std::ranges::distance(schematic.elements()), 0);
}

TEST(SchematicOld, SchematicSingleElement) {
    SchematicOld schematic;

    schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {5},
        .output_delays = std::vector<delay_t>(5, delay_t {1us}),
    });

    EXPECT_EQ(schematic.element_count(), 1);
    EXPECT_EQ(schematic.total_input_count(), 1);
    EXPECT_EQ(schematic.total_output_count(), 5);
    EXPECT_EQ(std::ranges::distance(schematic.elements()), 1);
}

TEST(SchematicOld, ElementProperties) {
    SchematicOld schematic;
    schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .output_delays = {element_output_delay(ElementType::and_element)},
    });

    const SchematicOld& schematic_const {schematic};
    const SchematicOld::ConstElement element {schematic_const.element(element_id_t {0})};

    EXPECT_EQ(element.element_id(), element_id_t {0});
    EXPECT_EQ(element.element_type(), ElementType::and_element);
    EXPECT_EQ(element.input_count(), connection_count_t {3});
    EXPECT_EQ(element.output_count(), connection_count_t {1});

    EXPECT_EQ(std::ranges::distance(element.inputs()), 3);
    EXPECT_EQ(std::ranges::distance(element.outputs()), 1);
}

TEST(SchematicOld, EqualityOperators) {
    SchematicOld schematic;

    auto wire = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {3},
        .output_delays = std::vector<delay_t>(3, delay_t {1us}),
    });
    auto inverter = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::buffer_element,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .output_delays = {element_output_delay(ElementType::buffer_element)},
    });

    const SchematicOld& schematic_const {schematic};

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
}

TEST(SchematicOld, ConnectionProperties) {
    SchematicOld schematic;

    auto wire = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {3},
        .output_delays = std::vector<delay_t>(3, delay_t {1us}),
    });
    auto and_element = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .output_delays = {element_output_delay(ElementType::and_element)},
    });

    auto id_1 = connection_id_t {1};

    EXPECT_EQ(wire.output(id_1).element_id(), wire.element_id());
    EXPECT_EQ(wire.output(id_1).output_index(), connection_id_t {1});
    EXPECT_EQ(wire.output(id_1).element(), wire);
    EXPECT_EQ(wire.output(id_1).has_connected_element(), false);

    EXPECT_EQ(and_element.input(id_1).element_id(), and_element.element_id());
    EXPECT_EQ(and_element.input(id_1).input_index(), connection_id_t {1});
    EXPECT_EQ(and_element.input(id_1).element(), and_element);
    EXPECT_EQ(and_element.input(id_1).has_connected_element(), false);
}

TEST(SchematicOld, ConnectedOutput) {
    SchematicOld schematic;

    auto wire = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {5},
        .output_delays = std::vector<delay_t>(5, delay_t {1us}),
    });
    auto and_element = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .output_delays = {element_output_delay(ElementType::and_element)},
    });

    auto id_1 = connection_id_t {1};
    wire.output(id_1).connect(and_element.input(id_1));

    EXPECT_EQ(wire.output(id_1).has_connected_element(), true);
    EXPECT_EQ(wire.output(id_1).connected_element_id(), and_element.element_id());
    EXPECT_EQ(wire.output(id_1).connected_element(), and_element);
    EXPECT_EQ(wire.output(id_1).connected_input(), and_element.input(id_1));

    EXPECT_EQ(and_element.input(id_1).has_connected_element(), true);
    EXPECT_EQ(and_element.input(id_1).connected_element_id(), wire.element_id());
    EXPECT_EQ(and_element.input(id_1).connected_element(), wire);
    EXPECT_EQ(and_element.input(id_1).connected_output(), wire.output(id_1));
}

TEST(SchematicOld, ConnectInput) {
    SchematicOld schematic;

    auto wire = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {5},
        .output_delays = std::vector<delay_t>(5, delay_t {1us}),
    });
    auto and_element = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .output_delays = {element_output_delay(ElementType::and_element)},
    });

    auto id_1 = connection_id_t {1};
    and_element.input(id_1).connect(wire.output(id_1));

    EXPECT_EQ(wire.output(id_1).has_connected_element(), true);
    EXPECT_EQ(wire.output(id_1).connected_element_id(), and_element.element_id());
    EXPECT_EQ(wire.output(id_1).connected_element(), and_element);
    EXPECT_EQ(wire.output(id_1).connected_input(), and_element.input(id_1));

    EXPECT_EQ(and_element.input(id_1).has_connected_element(), true);
    EXPECT_EQ(and_element.input(id_1).connected_element_id(), wire.element_id());
    EXPECT_EQ(and_element.input(id_1).connected_element(), wire);
    EXPECT_EQ(and_element.input(id_1).connected_output(), wire.output(id_1));
}

TEST(SchematicOld, ClearedInput) {
    SchematicOld schematic;

    auto wire = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {5},
        .output_delays = std::vector<delay_t>(5, delay_t {1us}),
    });
    auto and_element = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .output_delays = {element_output_delay(ElementType::and_element)},
    });

    auto id_1 = connection_id_t {1};
    wire.output(id_1).connect(and_element.input(id_1));
    and_element.input(id_1).clear_connection();

    EXPECT_EQ(and_element.input(id_1).has_connected_element(), false);
    EXPECT_EQ(wire.output(id_1).has_connected_element(), false);
}

TEST(SchematicOld, ClearedOutput) {
    SchematicOld schematic;

    auto wire = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {5},
        .output_delays = std::vector<delay_t>(5, delay_t {1us}),
    });
    auto and_element = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .output_delays = {element_output_delay(ElementType::and_element)},
    });

    auto id_1 = connection_id_t {1};
    wire.output(id_1).connect(and_element.input(id_1));
    wire.output(id_1).clear_connection();

    EXPECT_EQ(and_element.input(id_1).has_connected_element(), false);
    EXPECT_EQ(wire.output(id_1).has_connected_element(), false);
}

TEST(SchematicOld, ReconnectInput) {
    SchematicOld schematic;

    auto wire = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {5},
        .output_delays = std::vector<delay_t>(5, delay_t {1us}),
    });
    auto and_element = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .output_delays = {element_output_delay(ElementType::and_element)},
    });
    auto inverter = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::buffer_element,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .output_delays = {element_output_delay(ElementType::buffer_element)},
    });

    auto id_0 = connection_id_t {0};
    wire.output(id_0).connect(and_element.input(id_0));
    and_element.input(id_0).connect(inverter.output(id_0));

    EXPECT_EQ(wire.output(id_0).has_connected_element(), false);
    EXPECT_EQ(and_element.input(id_0).has_connected_element(), true);
    EXPECT_EQ(inverter.output(id_0).has_connected_element(), true);
}

TEST(SchematicOld, ReconnectOutput) {
    SchematicOld schematic;

    auto wire = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {5},
        .output_delays = std::vector<delay_t>(5, delay_t {1us}),
    });
    auto and_element = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .output_delays = {element_output_delay(ElementType::and_element)},
    });
    auto or_element = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::or_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .output_delays = {element_output_delay(ElementType::or_element)},
    });

    auto id_1 = connection_id_t {1};
    wire.output(id_1).connect(and_element.input(id_1));
    wire.output(id_1).connect(or_element.input(id_1));

    EXPECT_EQ(wire.output(id_1).has_connected_element(), true);
    EXPECT_EQ(and_element.input(id_1).has_connected_element(), false);
    EXPECT_EQ(or_element.input(id_1).has_connected_element(), true);
}

TEST(SchematicOld, TestPlaceholders) {
    SchematicOld schematic;
    auto wire = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {5},
        .output_delays = std::vector<delay_t>(5, delay_t {1us}),
    });
    EXPECT_EQ(schematic.element_count(), 1);

    add_output_placeholders(schematic);
    EXPECT_EQ(schematic.element_count(), 6);

    EXPECT_EQ(wire.output(connection_id_t {3}).has_connected_element(), true);
    EXPECT_EQ(wire.output(connection_id_t {3}).connected_element().element_type(),
              ElementType::placeholder);
}

//
// Element View
//

TEST(SchematicOld, ElementViewEmpty) {
    SchematicOld schematic;

    auto view = SchematicOld::ElementView {schematic};

    ASSERT_THAT(view, testing::ElementsAre());
    ASSERT_EQ(std::empty(view), true);
    ASSERT_EQ(std::size(view), 0);
}

TEST(SchematicOld, ElementViewFull) {
    SchematicOld schematic;

    auto wire = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .output_delays = {delay_t {1us}},
    });
    auto inverter = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::buffer_element,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .output_delays = {element_output_delay(ElementType::buffer_element)},
    });

    auto view = SchematicOld::ElementView {schematic};

    ASSERT_THAT(view, testing::ElementsAre(wire, inverter));
    ASSERT_EQ(std::empty(view), false);
    ASSERT_EQ(std::size(view), 2);
}

TEST(SchematicOld, ElementViewRanges) {
    SchematicOld schematic;
    schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .output_delays = {delay_t {1us}},
    });
    schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::buffer_element,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .output_delays = {element_output_delay(ElementType::buffer_element)},
    });

    auto view = SchematicOld::ElementView {schematic};

    ASSERT_EQ(std::distance(view.begin(), view.end()), 2);
    ASSERT_EQ(std::ranges::distance(std::ranges::begin(view), std::ranges::end(view)), 2);
    ASSERT_EQ(std::ranges::distance(view), 2);
}

//
// Element Inputs View
//

TEST(SchematicOld, InputsViewEmpty) {
    SchematicOld schematic;
    auto wire = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {0},
        .output_count = connection_count_t {1},
        .output_delays = {delay_t {1us}},
    });
    auto view = SchematicOld::InputView {wire};

    ASSERT_THAT(view, testing::ElementsAre());
    ASSERT_EQ(std::empty(view), true);
    ASSERT_EQ(std::size(view), 0);
}

TEST(SchematicOld, InputsViewFull) {
    SchematicOld schematic;
    auto and_element = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .output_delays = {element_output_delay(ElementType::and_element)},
    });
    auto view = SchematicOld::InputView {and_element};

    ASSERT_THAT(view, testing::ElementsAre(and_element.input(connection_id_t {0}),
                                           and_element.input(connection_id_t {1})));
    ASSERT_EQ(std::empty(view), false);
    ASSERT_EQ(std::size(view), 2);
}

TEST(SchematicOld, InputsViewRanges) {
    SchematicOld schematic;
    auto and_element = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .output_delays = {element_output_delay(ElementType::and_element)},
    });
    auto view = SchematicOld::InputView {and_element};

    ASSERT_EQ(std::distance(view.begin(), view.end()), 2);
    ASSERT_EQ(std::ranges::distance(std::ranges::begin(view), std::ranges::end(view)), 2);
    ASSERT_EQ(std::ranges::distance(view), 2);
}

}  // namespace logicsim
