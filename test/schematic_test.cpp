
#include "schematic.h"

#include "algorithm/to_vector.h"
#include "logic_item/schematic_info.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <ranges>

namespace logicsim {
TEST(Schematic, EmptySchematic) {
    const auto schematic = Schematic {};

    EXPECT_EQ(schematic.size(), 0);
    EXPECT_EQ(schematic.empty(), true);
    EXPECT_EQ(schematic.total_input_count(), 0);
    EXPECT_EQ(schematic.total_output_count(), 0);
    EXPECT_EQ(std::size(element_ids(schematic)), 0);
}

TEST(Schematic, SchematicSingleElement) {
    auto schematic = Schematic {};

    const auto element_id = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {5},
        .input_inverters = {false},
        .output_delays = output_delays_t(5, delay_t {1us}),
    });

    EXPECT_EQ(schematic.size(), 1);
    EXPECT_EQ(schematic.empty(), false);
    EXPECT_EQ(schematic.total_input_count(), 1);
    EXPECT_EQ(schematic.total_output_count(), 5);
    EXPECT_EQ(std::size(element_ids(schematic)), 1);

    EXPECT_EQ(bool {element_id}, true);
    EXPECT_EQ(std::size(input_ids(schematic, element_id)), 1);
    EXPECT_EQ(std::size(output_ids(schematic, element_id)), 5);
    EXPECT_EQ(std::size(inputs(schematic, element_id)), 1);
    EXPECT_EQ(std::size(outputs(schematic, element_id)), 5);
}

TEST(Schematic, ElementProperties) {
    auto schematic = Schematic {};

    const auto input_inverters = logic_small_vector_t {false, true, false};
    const auto output_delays = output_delays_t {delay_t {5us}};

    const auto element_id = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},

        .sub_circuit_id = circuit_id_t {10},
        .input_inverters = input_inverters,
        .output_delays = output_delays,
        .history_length = delay_t {10us},
    });

    EXPECT_EQ(schematic.element_type(element_id), ElementType::and_element);
    EXPECT_EQ(schematic.input_count(element_id), connection_count_t {3});
    EXPECT_EQ(schematic.output_count(element_id), connection_count_t {1});

    EXPECT_EQ(schematic.sub_circuit_id(element_id), circuit_id_t {10});
    EXPECT_EQ(schematic.input_inverters(element_id), input_inverters);
    EXPECT_EQ(schematic.output_delays(element_id), output_delays);
    EXPECT_EQ(schematic.history_length(element_id), delay_t {10us});

    const auto id_0 = connection_id_t {0};
    const auto id_1 = connection_id_t {1};
    const auto id_2 = connection_id_t {2};

    EXPECT_EQ(schematic.output_delay(output_t {element_id, id_0}), delay_t {5us});
    EXPECT_EQ(schematic.input_inverted(input_t {element_id, id_0}), false);
    EXPECT_EQ(schematic.input_inverted(input_t {element_id, id_1}), true);
    EXPECT_EQ(schematic.input_inverted(input_t {element_id, id_2}), false);

    EXPECT_EQ(element_id, element_id_t {0});
    EXPECT_THAT(to_vector(element_ids(schematic)),
                testing::ElementsAre(element_id_t {0}));

    EXPECT_THAT(to_vector(inputs(schematic, element_id)),
                testing::ElementsAre(input_t {element_id_t {0}, connection_id_t {0}},
                                     input_t {element_id_t {0}, connection_id_t {1}},
                                     input_t {element_id_t {0}, connection_id_t {2}}));

    EXPECT_THAT(to_vector(outputs(schematic, element_id)),
                testing::ElementsAre(output_t {element_id_t {0}, connection_id_t {0}}));
}

TEST(Schematic, EqualityOperators) {
    const auto new_element_0 = schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {3},
        .input_inverters = {false},
        .output_delays = output_delays_t(3, delay_t {1us}),
    };
    const auto new_element_1 = schematic::NewElement {
        .element_type = ElementType::buffer_element,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .input_inverters = {false},
        .output_delays = {element_output_delay(LogicItemType::buffer_element)},
    };

    auto schematic_1 = Schematic {};
    schematic_1.add_element(schematic::NewElement {new_element_0});
    schematic_1.add_element(schematic::NewElement {new_element_1});
    ASSERT_EQ(schematic_1.size(), 2);

    auto schematic_2 = Schematic {};
    EXPECT_NE(schematic_1, schematic_2);
    schematic_2.add_element(schematic::NewElement {new_element_0});
    EXPECT_NE(schematic_1, schematic_2);
    schematic_2.add_element(schematic::NewElement {new_element_1});
    EXPECT_EQ(schematic_1, schematic_2);
}

TEST(Schematic, ConnectionPropertiesNotConnected) {
    auto schematic = Schematic {};

    const auto wire = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {3},
        .input_inverters = {false},
        .output_delays = output_delays_t(3, delay_t {1us}),
    });
    const auto and_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false, false},
        .output_delays = {element_output_delay(LogicItemType::and_element)},
    });

    const auto id_0 = connection_id_t {0};
    const auto id_1 = connection_id_t {1};
    const auto id_2 = connection_id_t {2};

    EXPECT_FALSE(schematic.output(input_t {wire, id_0}));
    EXPECT_FALSE(schematic.input(output_t {wire, id_0}));
    EXPECT_FALSE(schematic.input(output_t {wire, id_1}));
    EXPECT_FALSE(schematic.input(output_t {wire, id_2}));

    EXPECT_FALSE(schematic.output(input_t {and_element, id_0}));
    EXPECT_FALSE(schematic.output(input_t {and_element, id_1}));
    EXPECT_FALSE(schematic.output(input_t {and_element, id_2}));
    EXPECT_FALSE(schematic.input(output_t {and_element, id_0}));

    // wire
    for (const auto input : inputs(schematic, wire)) {
        EXPECT_FALSE(schematic.output(input));
    }
    for (const auto output : outputs(schematic, wire)) {
        EXPECT_FALSE(schematic.input(output));
    }

    // and_element
    for (const auto input : inputs(schematic, and_element)) {
        EXPECT_FALSE(schematic.output(input));
    }
    for (const auto output : outputs(schematic, and_element)) {
        EXPECT_FALSE(schematic.input(output));
    }
}

TEST(Schematic, ConnectedOutput) {
    auto schematic = Schematic {};

    auto wire = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {5},
        .input_inverters = {false},
        .output_delays = output_delays_t(5, delay_t {1us}),
    });
    auto and_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false, false},
        .output_delays = {element_output_delay(LogicItemType::and_element)},
    });

    const auto id_0 = connection_id_t {0};
    const auto id_1 = connection_id_t {1};
    const auto id_2 = connection_id_t {2};

    schematic.connect(output_t {wire, id_1}, input_t {and_element, id_1});

    EXPECT_FALSE(schematic.output(input_t {wire, id_0}));
    EXPECT_FALSE(schematic.input(output_t {wire, id_0}));
    EXPECT_EQ(schematic.input(output_t {wire, id_1}), input_t(and_element, id_1));
    EXPECT_FALSE(schematic.input(output_t {wire, id_2}));

    EXPECT_FALSE(schematic.output(input_t {and_element, id_0}));
    EXPECT_EQ(schematic.output(input_t {and_element, id_1}), output_t(wire, id_1));
    EXPECT_FALSE(schematic.output(input_t {and_element, id_2}));
    EXPECT_FALSE(schematic.input(output_t {and_element, id_0}));
}

TEST(Schematic, ConnectInput) {
    auto schematic = Schematic {};

    auto wire = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {5},
        .input_inverters = {false},
        .output_delays = output_delays_t(5, delay_t {1us}),
    });
    auto and_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false, false},
        .output_delays = {element_output_delay(LogicItemType::and_element)},
    });

    const auto id_0 = connection_id_t {0};
    const auto id_1 = connection_id_t {1};
    const auto id_2 = connection_id_t {2};

    schematic.connect(input_t {and_element, id_1}, output_t {wire, id_1});

    EXPECT_FALSE(schematic.output(input_t {wire, id_0}));
    EXPECT_FALSE(schematic.input(output_t {wire, id_0}));
    EXPECT_EQ(schematic.input(output_t {wire, id_1}), input_t(and_element, id_1));
    EXPECT_FALSE(schematic.input(output_t {wire, id_2}));

    EXPECT_FALSE(schematic.output(input_t {and_element, id_0}));
    EXPECT_EQ(schematic.output(input_t {and_element, id_1}), output_t(wire, id_1));
    EXPECT_FALSE(schematic.output(input_t {and_element, id_2}));
    EXPECT_FALSE(schematic.input(output_t {and_element, id_0}));
}

TEST(Schematic, ClearedInput) {
    auto schematic = Schematic {};

    auto wire = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {5},
        .input_inverters = {false},
        .output_delays = output_delays_t(5, delay_t {1us}),
    });
    auto and_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false, false},
        .output_delays = {element_output_delay(LogicItemType::and_element)},
    });

    auto id_1 = connection_id_t {1};

    schematic.connect(output_t {wire, id_1}, input_t {and_element, id_1});

    EXPECT_TRUE(schematic.input(output_t {wire, id_1}));
    EXPECT_TRUE(schematic.output(input_t {and_element, id_1}));

    schematic.clear(input_t {and_element, id_1});

    EXPECT_FALSE(schematic.input(output_t {wire, id_1}));
    EXPECT_FALSE(schematic.output(input_t {and_element, id_1}));
}

TEST(Schematic, ClearedOutput) {
    auto schematic = Schematic {};

    auto wire = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {5},
        .input_inverters = {false},
        .output_delays = output_delays_t(5, delay_t {1us}),
    });
    auto and_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false, false},
        .output_delays = {element_output_delay(LogicItemType::and_element)},
    });

    auto id_1 = connection_id_t {1};

    schematic.connect(output_t {wire, id_1}, input_t {and_element, id_1});

    EXPECT_TRUE(schematic.input(output_t {wire, id_1}));
    EXPECT_TRUE(schematic.output(input_t {and_element, id_1}));

    schematic.clear(output_t {wire, id_1});

    EXPECT_FALSE(schematic.input(output_t {wire, id_1}));
    EXPECT_FALSE(schematic.output(input_t {and_element, id_1}));
}

TEST(Schematic, ClearedAllWire) {
    auto schematic = Schematic {};

    auto wire = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {5},
        .input_inverters = {false},
        .output_delays = output_delays_t(5, delay_t {1us}),
    });
    auto and_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false, false},
        .output_delays = {element_output_delay(LogicItemType::and_element)},
    });

    auto id_1 = connection_id_t {1};

    schematic.connect(output_t {wire, id_1}, input_t {and_element, id_1});

    EXPECT_TRUE(schematic.input(output_t {wire, id_1}));
    EXPECT_TRUE(schematic.output(input_t {and_element, id_1}));

    schematic.clear_all_connections(wire);

    EXPECT_FALSE(schematic.input(output_t {wire, id_1}));
    EXPECT_FALSE(schematic.output(input_t {and_element, id_1}));
}

TEST(Schematic, ClearedAllElement) {
    auto schematic = Schematic {};

    auto wire = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {5},
        .input_inverters = {false},
        .output_delays = output_delays_t(5, delay_t {1us}),
    });
    auto and_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false, false},
        .output_delays = {element_output_delay(LogicItemType::and_element)},
    });

    auto id_1 = connection_id_t {1};

    schematic.connect(output_t {wire, id_1}, input_t {and_element, id_1});

    EXPECT_TRUE(schematic.input(output_t {wire, id_1}));
    EXPECT_TRUE(schematic.output(input_t {and_element, id_1}));

    schematic.clear_all_connections(and_element);

    EXPECT_FALSE(schematic.input(output_t {wire, id_1}));
    EXPECT_FALSE(schematic.output(input_t {and_element, id_1}));
}

TEST(Schematic, ReconnectInput) {
    auto schematic = Schematic {};

    auto wire_1 = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {5},
        .input_inverters = {false},
        .output_delays = output_delays_t(5, delay_t {1us}),
    });
    auto and_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false, false},
        .output_delays = {element_output_delay(LogicItemType::and_element)},
    });
    auto wire_2 = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {2},
        .input_inverters = {false},
        .output_delays = output_delays_t(2, delay_t {1us}),
    });

    auto id_0 = connection_id_t {0};

    schematic.connect(output_t {wire_1, id_0}, input_t {and_element, id_0});

    EXPECT_EQ(bool {schematic.input(output_t {wire_1, id_0})}, true);
    EXPECT_EQ(bool {schematic.output(input_t {and_element, id_0})}, true);
    EXPECT_EQ(bool {schematic.input(output_t {wire_2, id_0})}, false);

    schematic.connect(input_t {and_element, id_0}, output_t {wire_2, id_0});

    EXPECT_EQ(bool {schematic.input(output_t {wire_1, id_0})}, false);
    EXPECT_EQ(bool {schematic.output(input_t {and_element, id_0})}, true);
    EXPECT_EQ(bool {schematic.input(output_t {wire_2, id_0})}, true);
}

TEST(Schematic, ReconnectOutput) {
    auto schematic = Schematic {};

    auto wire = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {5},
        .input_inverters = {false},
        .output_delays = output_delays_t(5, delay_t {1us}),
    });
    auto and_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false, false},
        .output_delays = {element_output_delay(LogicItemType::and_element)},
    });
    auto or_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::or_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false},
        .output_delays = {element_output_delay(LogicItemType::or_element)},
    });

    auto id_0 = connection_id_t {0};

    schematic.connect(output_t {wire, id_0}, input_t {and_element, id_0});

    EXPECT_EQ(bool {schematic.input(output_t {wire, id_0})}, true);
    EXPECT_EQ(bool {schematic.output(input_t {and_element, id_0})}, true);
    EXPECT_EQ(bool {schematic.output(input_t {or_element, id_0})}, false);

    schematic.connect(output_t {wire, id_0}, input_t {or_element, id_0});

    EXPECT_EQ(bool {schematic.input(output_t {wire, id_0})}, true);
    EXPECT_EQ(bool {schematic.output(input_t {and_element, id_0})}, false);
    EXPECT_EQ(bool {schematic.output(input_t {or_element, id_0})}, true);
}

}  // namespace logicsim
