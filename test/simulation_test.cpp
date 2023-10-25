
#include "simulation.h"

#include "component/simulation/history_view.h"
#include "logging.h"
#include "logic_item/schematic_info.h"
#include "schematic.h"
#include "schematic_generation.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>

namespace logicsim {

TEST(SimulationTest, InitializeSimulation) {
    Schematic schematic;
    const auto inverter = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::buffer_element,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .input_inverters = logic_small_vector_t {true},
        .output_delays = {element_output_delay(ElementType::buffer_element)},
    });
    add_missing_placeholders(schematic);

    auto simulation = Simulation {std::move(schematic)};
    simulation.run();

    EXPECT_EQ(simulation.input_value(input_t {inverter, connection_id_t {0}}), false);
    EXPECT_EQ(simulation.output_value(output_t {inverter, connection_id_t {0}}), true);
    EXPECT_EQ(simulation.is_finished(), true);
}

TEST(SimulationTest, SimulationTimeAdvancingWithoutEvents) {
    auto simulation = Simulation {Schematic {}};

    EXPECT_EQ(simulation.time(), time_t {0us});
    simulation.run({.simulate_for = delay_t {3ms}});
    EXPECT_EQ(simulation.time(), time_t {3ms});
}

TEST(SimulationTest, SimulationProcessAllEvents) {
    Schematic schematic;
    const auto and_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false},
        .output_delays = {element_output_delay(ElementType::and_element)},
    });
    const auto xor_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::xor_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false},
        .output_delays = {element_output_delay(ElementType::xor_element)},
    });
    auto simulation = Simulation {std::move(schematic)};

    const auto id_0 = connection_id_t {0};

    EXPECT_EQ(simulation.processed_event_count(), 0);

    simulation.set_unconnected_input(input_t {and_element, id_0}, true);
    simulation.set_unconnected_input(input_t {xor_element, id_0}, true);
    simulation.run();

    EXPECT_EQ(simulation.processed_event_count(), 2);
    EXPECT_EQ(simulation.is_finished(), true);
}

TEST(SimulationTest, SimulationTimeAdvancingWithNoEvents) {
    using namespace std::chrono_literals;

    Schematic schematic;
    schematic.add_element(schematic::NewElement {
        .element_type = ElementType::buffer_element,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .input_inverters = {true},
        .output_delays = {element_output_delay(ElementType::buffer_element)},
    });

    auto simulation = Simulation {std::move(schematic)};

    EXPECT_EQ(simulation.time(), time_t {0us});
    simulation.run({.simulate_for = delay_t {5ms}});
    EXPECT_EQ(simulation.time(), time_t {5ms});

    EXPECT_EQ(simulation.is_finished(), true);
}

TEST(SimulationTest, SimulationTimeAdvancingWithInfiniteEvents) {
    using namespace std::chrono_literals;

    // create infinite loop
    Schematic schematic;
    const auto inverter = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::buffer_element,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .input_inverters = {true},
        .output_delays = {element_output_delay(ElementType::buffer_element)},
    });
    const auto wire = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .input_inverters = {false},
        .output_delays = {delay_t {100us}},
    });

    const auto id_0 = connection_id_t {0};
    schematic.connect(output_t {inverter, id_0}, input_t {wire, id_0});
    schematic.connect(output_t {wire, id_0}, input_t {inverter, id_0});

    auto simulation = Simulation {std::move(schematic)};

    EXPECT_EQ(simulation.time(), time_t {0us});
    simulation.run({.simulate_for = delay_t {5ms}});
    EXPECT_EQ(simulation.time(), time_t {5ms});

    EXPECT_EQ(simulation.is_finished(), false);
}

TEST(SimulationTest, SimulationInfiniteEventsTimeout) {
    using namespace std::chrono_literals;

    // create infinite loop
    Schematic schematic;
    auto inverter = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::buffer_element,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .input_inverters = {true},
        .output_delays = {element_output_delay(ElementType::buffer_element)},
    });
    auto wire = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .input_inverters = {false},
        .output_delays = {delay_t {1us}},
    });

    const auto id_0 = connection_id_t {0};
    schematic.connect(output_t {inverter, id_0}, input_t {wire, id_0});
    schematic.connect(output_t {wire, id_0}, input_t {inverter, id_0});

    auto simulation = Simulation {std::move(schematic)};

    // run simulation for 5 ms
    EXPECT_EQ(simulation.time(), time_t {0us});
    const auto start = std::chrono::steady_clock::now();
    simulation.run({.realtime_timeout = 5ms});
    const auto end = std::chrono::steady_clock::now();

    EXPECT_GT(simulation.time(), time_t {1ms});
    const auto delay = end - start;

#ifndef NDEBUG
    // debug
    ASSERT_THAT(delay > 4ms, true);
    ASSERT_THAT(delay < 20ms, true);
#else
    // release
    ASSERT_THAT(delay > 4.5ms, true);
    ASSERT_THAT(delay < 5.5ms, true);
#endif
}

TEST(SimulationTest, AdditionalEvents) {
    Schematic schematic;
    auto xor_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::xor_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false},
        .output_delays = {element_output_delay(ElementType::xor_element)},
    });
    add_missing_placeholders(schematic);

    auto simulation = Simulation {std::move(schematic)};
    simulation.run();

    const auto input_0 = input_t {xor_element, connection_id_t {0}};
    const auto input_1 = input_t {xor_element, connection_id_t {1}};
    const auto output_0 = output_t {xor_element, connection_id_t {0}};

    EXPECT_EQ(simulation.input_value(input_0), false);
    EXPECT_EQ(simulation.input_value(input_1), false);
    EXPECT_EQ(simulation.output_value(output_0), false);

    // enable first input
    simulation.set_unconnected_input(input_0, true);
    simulation.run();

    EXPECT_EQ(simulation.input_value(input_0), true);
    EXPECT_EQ(simulation.input_value(input_1), false);
    EXPECT_EQ(simulation.output_value(output_0), true);

    // enable second input
    simulation.set_unconnected_input(input_1, true);
    simulation.run();

    EXPECT_EQ(simulation.input_value(input_0), true);
    EXPECT_EQ(simulation.input_value(input_1), true);
    EXPECT_EQ(simulation.output_value(output_0), false);
}

TEST(SimulationTest, SimultaneousEvents) {
    Schematic schematic;
    const auto wire = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {2},
        .input_inverters = {false},
        .output_delays = {delay_t {100us}, delay_t {100us}},
    });
    auto xor_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::xor_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .input_inverters = {true, false},
        .output_delays = {element_output_delay(ElementType::xor_element)},
    });
    const auto id_0 = connection_id_t {0};
    const auto id_1 = connection_id_t {1};
    schematic.connect(output_t {wire, id_0}, input_t {xor_element, id_0});
    schematic.connect(output_t {wire, id_1}, input_t {xor_element, id_1});
    add_missing_placeholders(schematic);

    const auto wire_0 = input_t {wire, connection_id_t {0}};
    const auto input_0 = input_t {xor_element, connection_id_t {0}};
    const auto input_1 = input_t {xor_element, connection_id_t {1}};
    const auto output_0 = output_t {xor_element, connection_id_t {0}};

    auto simulation = Simulation {std::move(schematic)};
    simulation.run();

    EXPECT_EQ(simulation.input_value(wire_0), false);
    EXPECT_EQ(simulation.input_value(input_0), false);
    EXPECT_EQ(simulation.input_value(input_1), false);
    EXPECT_EQ(simulation.output_value(output_0), true);

    simulation.set_unconnected_input(wire_0, true);
    simulation.run();

    EXPECT_EQ(simulation.input_value(wire_0), true);
    EXPECT_EQ(simulation.input_value(input_0), true);
    EXPECT_EQ(simulation.input_value(input_1), true);
    EXPECT_EQ(simulation.output_value(output_0), true);

    simulation.set_unconnected_input(wire_0, false);
    simulation.run();

    EXPECT_EQ(simulation.input_value(wire_0), false);
    EXPECT_EQ(simulation.input_value(input_0), false);
    EXPECT_EQ(simulation.input_value(input_1), false);
    EXPECT_EQ(simulation.output_value(output_0), true);
}

TEST(SimulationTest, HalfAdder) {
    Schematic schematic;

    auto input0 = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {2},
        .input_inverters = {false},
        .output_delays = {delay_t {1us}, delay_t {1us}},
    });
    auto input1 = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {2},
        .input_inverters = {false},
        .output_delays = {delay_t {1us}, delay_t {1us}},
    });
    auto carry = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false},
        .output_delays = {element_output_delay(ElementType::and_element)},
    });
    auto output = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::xor_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false},
        .output_delays = {element_output_delay(ElementType::xor_element)},
    });

    const auto id_0 = connection_id_t {0};
    const auto id_1 = connection_id_t {1};

    schematic.connect(output_t {input0, id_0}, input_t {carry, id_0});
    schematic.connect(output_t {input0, id_1}, input_t {output, id_0});

    schematic.connect(output_t {input1, id_0}, input_t {carry, id_1});
    schematic.connect(output_t {input1, id_1}, input_t {output, id_1});

    add_missing_placeholders(schematic);

    auto simulation = Simulation {std::move(schematic)};

    // 0 + 0 -> 00
    {
        simulation.set_unconnected_input(input_t {input0, id_0}, false);
        simulation.set_unconnected_input(input_t {input1, id_0}, false);
        simulation.run();

        EXPECT_EQ(simulation.output_value(output_t {output, id_0}), false);
        EXPECT_EQ(simulation.output_value(output_t {carry, id_0}), false);
    }

    // 0 + 1 = 01
    {
        simulation.set_unconnected_input(input_t {input0, id_0}, true);
        simulation.set_unconnected_input(input_t {input1, id_0}, false);
        simulation.run();

        EXPECT_EQ(simulation.output_value(output_t {output, id_0}), true);
        EXPECT_EQ(simulation.output_value(output_t {carry, id_0}), false);
    }

    // 1 + 0 = 01
    {
        simulation.set_unconnected_input(input_t {input0, id_0}, false);
        simulation.set_unconnected_input(input_t {input1, id_0}, true);
        simulation.run();

        EXPECT_EQ(simulation.output_value(output_t {output, id_0}), true);
        EXPECT_EQ(simulation.output_value(output_t {carry, id_0}), false);
    }

    // 1 + 1 = 10
    {
        simulation.set_unconnected_input(input_t {input0, id_0}, true);
        simulation.set_unconnected_input(input_t {input1, id_0}, true);
        simulation.run();

        EXPECT_EQ(simulation.output_value(output_t {output, id_0}), false);
        EXPECT_EQ(simulation.output_value(output_t {carry, id_0}), true);
    }
}

TEST(SimulationTest, OutputDelayTest) {
    using namespace std::chrono_literals;

    Schematic schematic;
    auto wire = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {3},
        .input_inverters = {false},
        .output_delays = {delay_t {1ms}, delay_t {2ms}, delay_t {3ms}},
    });
    auto and_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false, false},
        .output_delays = {element_output_delay(ElementType::and_element)},
    });
    const auto id_0 = connection_id_t {0};
    const auto id_1 = connection_id_t {1};
    const auto id_2 = connection_id_t {2};
    schematic.connect(output_t {wire, id_0}, input_t {and_element, id_0});
    schematic.connect(output_t {wire, id_1}, input_t {and_element, id_1});
    schematic.connect(output_t {wire, id_2}, input_t {and_element, id_2});

    auto simulation = Simulation {std::move(schematic)};

    simulation.set_unconnected_input(input_t {wire, id_0}, true);

    // after 0.5 ms
    simulation.run({.simulate_for = delay_t {500us}});
    ASSERT_THAT(simulation.output_values(wire), testing::ElementsAre(0, 0, 0));
    // after 1.5 ms
    simulation.run({.simulate_for = delay_t {1ms}});
    ASSERT_THAT(simulation.output_values(wire), testing::ElementsAre(1, 0, 0));
    // after 2.5 ms
    simulation.run({.simulate_for = delay_t {1ms}});
    ASSERT_THAT(simulation.output_values(wire), testing::ElementsAre(1, 1, 0));
    // after 3.5 ms
    simulation.run({.simulate_for = delay_t {1ms}});
    ASSERT_THAT(simulation.output_values(wire), testing::ElementsAre(1, 1, 1));
}

TEST(SimulationTest, JKFlipFlop) {
    using namespace std::chrono_literals;

    Schematic schematic;
    const auto flipflop = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::flipflop_jk,
        .input_count = connection_count_t {5},
        .output_count = connection_count_t {2},
        .input_inverters = {false, false, false, false, false},
        .output_delays = {element_output_delay(ElementType::flipflop_jk),
                          element_output_delay(ElementType::flipflop_jk)},
    });
    add_missing_placeholders(schematic);

    const auto input_clk = input_t {flipflop, connection_id_t {0}};
    const auto input_j = input_t {flipflop, connection_id_t {1}};
    const auto input_k = input_t {flipflop, connection_id_t {2}};
    const auto input_set = input_t {flipflop, connection_id_t {3}};
    const auto input_reset = input_t {flipflop, connection_id_t {4}};

    auto simulation = Simulation {std::move(schematic)};

    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(0, 1));

    // switch to j state
    simulation.set_unconnected_input(input_j, true);
    simulation.set_unconnected_input(input_clk, true);
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(0, 1));
    simulation.set_unconnected_input(input_clk, false);
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(1, 0));

    // switch to k state
    simulation.set_unconnected_input(input_j, false);
    simulation.set_unconnected_input(input_k, true);
    simulation.set_unconnected_input(input_clk, true);
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(1, 0));
    simulation.set_unconnected_input(input_clk, false);
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(0, 1));

    // toggle state
    simulation.set_unconnected_input(input_j, true);
    simulation.set_unconnected_input(input_k, true);
    simulation.set_unconnected_input(input_clk, true);
    simulation.set_unconnected_input(input_clk, false);
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(1, 0));
    simulation.set_unconnected_input(input_clk, true);
    simulation.set_unconnected_input(input_clk, false);
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(0, 1));

    // steady state
    simulation.set_unconnected_input(input_j, false);
    simulation.set_unconnected_input(input_k, false);
    simulation.set_unconnected_input(input_clk, true);
    simulation.set_unconnected_input(input_clk, false);
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(0, 1));

    // set
    simulation.set_unconnected_input(input_set, true);
    simulation.set_unconnected_input(input_set, false);
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(1, 0));

    // reset
    simulation.set_unconnected_input(input_reset, true);
    simulation.set_unconnected_input(input_reset, false);
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(0, 1));

    // reset wins
    simulation.set_unconnected_input(input_set, true);
    simulation.set_unconnected_input(input_reset, true);
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(0, 1));
}

TEST(SimulationTest, AndInputInverters1) {
    using namespace std::chrono_literals;

    Schematic schematic;
    const auto and_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .input_inverters = {true, true},
        .output_delays = {element_output_delay(ElementType::and_element)},
    });
    add_missing_placeholders(schematic);

    auto simulation = Simulation {std::move(schematic)};

    const auto id_0 = connection_id_t {0};

    simulation.run();
    ASSERT_THAT(simulation.input_values(and_element), testing::ElementsAre(false, false));
    ASSERT_THAT(simulation.output_values(and_element), testing::ElementsAre(true));

    simulation.set_unconnected_input(input_t {and_element, id_0}, true);
    simulation.run();
    ASSERT_THAT(simulation.input_values(and_element), testing::ElementsAre(true, false));
    ASSERT_THAT(simulation.output_values(and_element), testing::ElementsAre(false));
}

TEST(SimulationTest, AndInputInverters2) {
    using namespace std::chrono_literals;

    Schematic schematic;
    const auto and_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .input_inverters = {false, true},
        .output_delays = {element_output_delay(ElementType::and_element)},
    });
    add_missing_placeholders(schematic);

    auto simulation = Simulation {std::move(schematic)};

    const auto id_0 = connection_id_t {0};

    simulation.run();
    ASSERT_THAT(simulation.input_values(and_element), testing::ElementsAre(false, false));
    ASSERT_THAT(simulation.output_values(and_element), testing::ElementsAre(false));

    simulation.set_unconnected_input(input_t {and_element, id_0}, true);
    simulation.run();
    ASSERT_THAT(simulation.input_values(and_element), testing::ElementsAre(true, false));
    ASSERT_THAT(simulation.output_values(and_element), testing::ElementsAre(true));
}

TEST(SimulationTest, TestSetInputDelay) {
    Schematic schematic;
    const auto wire = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::buffer_element,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .input_inverters = {false},
        .output_delays = {delay_t {100ns}},
    });
    add_missing_placeholders(schematic);

    const auto id_0 = connection_id_t {0};

    auto simulation = Simulation {std::move(schematic)};

    simulation.run();
    ASSERT_EQ(simulation.time(), time_t {0ns});
    ASSERT_EQ(simulation.input_value(input_t {wire, id_0}), false);
    ASSERT_EQ(simulation.output_value(output_t {wire, id_0}), false);

    // auto advanced by 1ns when setting input
    simulation.set_unconnected_input(input_t {wire, id_0}, true);
    ASSERT_EQ(simulation.time(), time_t {1ns});
    ASSERT_EQ(simulation.input_value(input_t {wire, id_0}), false);
    ASSERT_EQ(simulation.output_value(output_t {wire, id_0}), false);

    // effect visible after another 2ns
    simulation.run({.simulate_for = delay_t {1ns}});
    ASSERT_EQ(simulation.time(), time_t {2ns});
    ASSERT_EQ(simulation.input_value(input_t {wire, id_0}), true);
    ASSERT_EQ(simulation.output_value(output_t {wire, id_0}), false);

    // output affected at 102ns
    simulation.run({.simulate_for = delay_t {99ns}});
    ASSERT_EQ(simulation.time(), time_t {101ns});
    ASSERT_EQ(simulation.input_value(input_t {wire, id_0}), true);
    ASSERT_EQ(simulation.output_value(output_t {wire, id_0}), false);

    simulation.run({.simulate_for = delay_t {1ns}});
    ASSERT_EQ(simulation.time(), time_t {102ns});
    ASSERT_EQ(simulation.input_value(input_t {wire, id_0}), true);
    ASSERT_EQ(simulation.output_value(output_t {wire, id_0}), true);

    // simulation finished
    ASSERT_EQ(simulation.is_finished(), true);
    simulation.run();
    ASSERT_EQ(simulation.time(), time_t {102ns});
}

TEST(SimulationTest, TestInputHistory) {
    Schematic schematic;
    const auto wire = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {2},
        .input_inverters = {false},
        .output_delays = {delay_t {10us}, delay_t {100us}},
        .history_length = delay_t {100us},
    });

    auto simulation = Simulation {std::move(schematic)};

    const auto wire_0 = input_t {wire, connection_id_t {0}};

    simulation.run();
    ASSERT_EQ(simulation.time(), time_t {0us});

    using entry_t = simulation::history_entry_t;
    ASSERT_THAT(simulation.input_history(wire),
                testing::ElementsAre(entry_t {time_t::min(), time_t {0us}, false}));

    // takes 2ns for input changes to take effect
    const auto eps = 2 * delay_t::epsilon();

    simulation.run({.simulate_for = time_t {10us} - simulation.time() - eps});
    simulation.set_unconnected_input(wire_0, true);
    simulation.run({.simulate_for = time_t {20us} - simulation.time() - eps});
    simulation.set_unconnected_input(wire_0, true);  // ignored
    simulation.run({.simulate_for = time_t {40us} - simulation.time() - eps});
    simulation.set_unconnected_input(wire_0, false);
    simulation.run({.simulate_for = time_t {60us} - simulation.time() - eps});
    simulation.set_unconnected_input(wire_0, true);

    simulation.run({.simulate_for = time_t {100us} - simulation.time()});
    ASSERT_EQ(simulation.time(), time_t {100us});
    ASSERT_THAT(simulation.input_history(wire),
                testing::ElementsAre(entry_t {time_t::min(), time_t {10us}, false},
                                     entry_t {time_t {10us}, time_t {40us}, true},
                                     entry_t {time_t {40us}, time_t {60us}, false},
                                     entry_t {time_t {60us}, time_t {100us}, true}));

    simulation.run({.simulate_for = time_t {180us} - simulation.time() - eps});
    simulation.set_unconnected_input(wire_0, false);

    simulation.run({.simulate_for = time_t {200us} - simulation.time()});
    ASSERT_EQ(simulation.time(), time_t {200us});
    ASSERT_THAT(simulation.input_history(wire),
                testing::ElementsAre(entry_t {time_t::min(), time_t {180us}, true},
                                     entry_t {time_t {180us}, time_t {200us}, false}));
}

/*
TEST(SimulationTest, TestClockGenerator) {
    using namespace std::chrono_literals;

    Schematic schematic;
    auto clock = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::clock_generator,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {3},
        .input_inverters = {false, false, false},
        .output_delays = {delay_t {100us}, delay_t {100us}, delay_t {100us}},
    });
    const auto id_0 = connection_id_t {0};
    const auto id_1 = connection_id_t {1};
    const auto id_2 = connection_id_t {2};
    schematic.connect(output_t {clock, id_1}, input_t {clock, id_1});
    schematic.connect(output_t {clock, id_2}, input_t {clock, id_2});

    auto simulation = Simulation {std::move(schematic)};

    simulation.initialize();
    simulation.submit_event(input_t {clock, id_0}, delay_t {50us}, true);

    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.output_value(output_t {clock, id_0}), false);
    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.output_value(output_t {clock, id_0}), true);
    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.output_value(output_t {clock, id_0}), false);
    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.output_value(output_t {clock, id_0}), true);
}

TEST(SimulationTest, TestClockGeneratorDifferentDelay) {
    using namespace std::chrono_literals;

    Schematic schematic;
    auto clock = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::clock_generator,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {3},
        .input_inverters = {false, false, false},
        .output_delays = {delay_t {100us}, delay_t {500us}, delay_t {500us}},
    });
    const auto id_0 = connection_id_t {0};
    const auto id_1 = connection_id_t {1};
    const auto id_2 = connection_id_t {2};
    schematic.connect(output_t {clock, id_1}, input_t {clock, id_1});
    schematic.connect(output_t {clock, id_2}, input_t {clock, id_2});

    auto simulation = Simulation {std::move(schematic)};

    simulation.initialize();
    simulation.submit_event(input_t {clock, id_0}, delay_t {50us}, true);

    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.output_value(output_t {clock, id_0}), false);
    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.output_value(output_t {clock, id_0}), true);
    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.output_value(output_t {clock, id_0}), true);
    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.output_value(output_t {clock, id_0}), true);
    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.output_value(output_t {clock, id_0}), true);
    simulation.run(delay_t {100us});  // 600 us
    ASSERT_EQ(simulation.output_value(output_t {clock, id_0}), true);
    simulation.run(delay_t {100us});  // 700 us
    ASSERT_EQ(simulation.output_value(output_t {clock, id_0}), false);
}

TEST(SimulationTest, TestClockReset) {
    using namespace std::chrono_literals;

    Schematic schematic;
    auto clock = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::clock_generator,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {3},
        .input_inverters = {false, false, false},
        .output_delays = {delay_t {1ns}, delay_t {1ms}, delay_t {1ms}},
    });
    const auto id_0 = connection_id_t {0};
    const auto id_1 = connection_id_t {1};
    const auto id_2 = connection_id_t {2};
    schematic.connect(output_t {clock, id_1}, input_t {clock, id_1});
    schematic.connect(output_t {clock, id_2}, input_t {clock, id_2});

    auto simulation = Simulation {std::move(schematic)};

    simulation.initialize();
    simulation.submit_event(input_t {clock, id_0}, delay_t {1000us}, true);
    simulation.submit_event(input_t {clock, id_0}, delay_t {1100us}, false);
    simulation.run(delay_t {10ns});

    ASSERT_EQ(simulation.output_value(output_t {clock, id_0}), false);
    simulation.run(delay_t {1ms});
    ASSERT_EQ(simulation.output_value(output_t {clock, id_0}), true);

    simulation.run(delay_t {999us});
    ASSERT_EQ(simulation.output_value(output_t {clock, id_0}), true);
    simulation.run(delay_t {1us});
    ASSERT_EQ(simulation.output_value(output_t {clock, id_0}), false);

    simulation.run(delay_t {1ms});
    ASSERT_EQ(simulation.output_value(output_t {clock, id_0}), false);
    simulation.run(delay_t {1ms});
    ASSERT_EQ(simulation.output_value(output_t {clock, id_0}), false);
    simulation.run(delay_t {1ms});
    ASSERT_EQ(simulation.output_value(output_t {clock, id_0}), false);
}

TEST(SimulationTest, TestShiftRegister) {
    using namespace std::chrono_literals;

    Schematic schematic;
    auto shift_register = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::shift_register,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {2},
        .input_inverters = {false, false, false},
        .output_delays = {element_output_delay(ElementType::shift_register),
                          element_output_delay(ElementType::shift_register)},
    });

    auto simulation = Simulation {std::move(schematic)};
    simulation.initialize();

    const auto get_relevant_state = [&]() {
        const auto& state = simulation.internal_state(shift_register);
        return logic_small_vector_t(state.begin() + 2, state.end());
    };

    simulation.run();
    // initial state
    ASSERT_THAT(simulation.output_values(shift_register), testing::ElementsAre(0, 0));
    ASSERT_THAT(get_relevant_state(), testing::ElementsAre(0, 0, 0, 0, 0, 0, 0, 0));

    // insert first element
    simulation.submit_events(shift_register, delay_t {1ms}, {true, true, false});
    simulation.submit_events(shift_register, delay_t {2ms}, {false, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(shift_register), testing::ElementsAre(0, 0));
    ASSERT_THAT(get_relevant_state(), testing::ElementsAre(1, 0, 0, 0, 0, 0, 0, 0));

    // insert second element
    simulation.submit_events(shift_register, delay_t {1ms}, {true, false, true});
    simulation.submit_events(shift_register, delay_t {2ms}, {false, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(shift_register), testing::ElementsAre(0, 0));
    ASSERT_THAT(get_relevant_state(), testing::ElementsAre(0, 1, 1, 0, 0, 0, 0, 0));

    // insert third element
    simulation.submit_events(shift_register, delay_t {1ms}, {true, true, true});
    simulation.submit_events(shift_register, delay_t {2ms}, {false, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(shift_register), testing::ElementsAre(0, 0));
    ASSERT_THAT(get_relevant_state(), testing::ElementsAre(1, 1, 0, 1, 1, 0, 0, 0));

    // insert forth element  &  receive first element
    simulation.submit_events(shift_register, delay_t {1ms}, {true, false, false});
    simulation.submit_events(shift_register, delay_t {2ms}, {false, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(shift_register), testing::ElementsAre(1, 0));
    ASSERT_THAT(get_relevant_state(), testing::ElementsAre(0, 0, 1, 1, 0, 1, 1, 0));

    // receive second element
    simulation.submit_events(shift_register, delay_t {1ms}, {true, false, false});
    simulation.submit_events(shift_register, delay_t {2ms}, {false, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(shift_register), testing::ElementsAre(0, 1));
    ASSERT_THAT(get_relevant_state(), testing::ElementsAre(0, 0, 0, 0, 1, 1, 0, 1));

    // receive third element
    simulation.submit_events(shift_register, delay_t {1ms}, {true, false, false});
    simulation.submit_events(shift_register, delay_t {2ms}, {false, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(shift_register), testing::ElementsAre(1, 1));
    ASSERT_THAT(get_relevant_state(), testing::ElementsAre(0, 0, 0, 0, 0, 0, 1, 1));

    // receive fourth element
    simulation.submit_events(shift_register, delay_t {1ms}, {true, false, false});
    simulation.submit_events(shift_register, delay_t {2ms}, {false, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(shift_register), testing::ElementsAre(0, 0));
    ASSERT_THAT(get_relevant_state(), testing::ElementsAre(0, 0, 0, 0, 0, 0, 0, 0));
}

*/
}  // namespace logicsim
