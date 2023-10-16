
#include "simulation.h"

#include "schematic_validation.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>

namespace logicsim {

// SimulationEvent

TEST(SimulationEventTest, EqualOperatorTest) {
    SimulationEvent event1 {time_t {123us}, element_id_t {1}, connection_id_t {2}, true};
    SimulationEvent event2 {time_t {123us}, element_id_t {1}, connection_id_t {2}, true};
    EXPECT_TRUE(event1 == event2);

    SimulationEvent event3 {time_t {123us}, element_id_t {1}, connection_id_t {3}, true};
    SimulationEvent event4 {time_t {123us}, element_id_t {1}, connection_id_t {2}, false};
    EXPECT_TRUE(event3 == event4);
}

TEST(SimulationEventTest, LessThanOperatorTest) {
    SimulationEvent event1 {time_t {123us}, element_id_t {1}, connection_id_t {2}, true};
    SimulationEvent event2 {time_t {789us}, element_id_t {3}, connection_id_t {4}, false};
    EXPECT_TRUE(event1 < event2);

    SimulationEvent event3 {time_t {123us}, element_id_t {1}, connection_id_t {4}, true};
    SimulationEvent event4 {time_t {123us}, element_id_t {3}, connection_id_t {2}, false};
    EXPECT_TRUE(event3 < event4);
}

TEST(SimulationEventTest, NotEqualOperatorTest) {
    SimulationEvent event1 {time_t {123us}, element_id_t {1}, connection_id_t {2}, true};
    SimulationEvent event2 {time_t {789us}, element_id_t {3}, connection_id_t {4}, false};
    EXPECT_TRUE(event1 != event2);
}

TEST(SimulationEventTest, GreaterThanOperatorTest) {
    SimulationEvent event1 {time_t {123us}, element_id_t {1}, connection_id_t {2}, true};
    SimulationEvent event2 {time_t {789us}, element_id_t {3}, connection_id_t {4}, false};
    EXPECT_TRUE(event2 > event1);
}

TEST(SimulationEventTest, LessThanOrEqualOperatorTest) {
    SimulationEvent event1 {time_t {123us}, element_id_t {1}, connection_id_t {2}, true};
    SimulationEvent event2 {time_t {789us}, element_id_t {3}, connection_id_t {4}, false};
    EXPECT_TRUE(event1 <= event2);
}

TEST(SimulationEventTest, GreaterThanOrEqualOperatorTest) {
    SimulationEvent event1 {time_t {123us}, element_id_t {1}, connection_id_t {2}, true};
    SimulationEvent event2 {time_t {789us}, element_id_t {3}, connection_id_t {4}, false};
    EXPECT_TRUE(event2 >= event1);
}

// Simulation

[[nodiscard]] auto get_uninitialized_simulation(Schematic& schematic) -> Simulation {
    add_output_placeholders(schematic);
    validate(schematic, schematic::validate_all);

    return Simulation {schematic};
}

[[nodiscard]] auto get_initialized_simulation(Schematic& schematic) -> Simulation {
    add_output_placeholders(schematic);
    validate(schematic, schematic::validate_all);

    Simulation simulation {schematic};
    simulation.initialize();
    return simulation;
}

TEST(SimulationTest, InitializeSimulation) {
    Schematic schematic;
    auto inverter = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::buffer_element,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .input_inverters = logic_small_vector_t {true},
        .output_delays = {defaults::logic_item_delay},
    });

    auto simulation = get_initialized_simulation(schematic);
    simulation.run();

    EXPECT_EQ(simulation.input_value(inverter.input(connection_id_t {0})), false);
    EXPECT_EQ(simulation.output_value(inverter.output(connection_id_t {0})), true);
}

TEST(SimulationTest, SimulationTimeAdvancingWithoutEvents) {
    using namespace std::chrono_literals;
    Schematic schematic;

    auto simulation = get_initialized_simulation(schematic);

    EXPECT_EQ(simulation.time(), time_t {0us});
    simulation.run(delay_t {3s});
    EXPECT_EQ(simulation.time(), time_t {3s});
}

TEST(SimulationTest, SimulationProcessAllEventsForTime) {
    using namespace std::chrono_literals;

    Schematic schematic;
    auto and_element = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .output_delays = {defaults::logic_item_delay},
    });
    auto xor_element = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::xor_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .output_delays = {defaults::logic_item_delay},
    });
    auto simulation = get_initialized_simulation(schematic);

    simulation.submit_event(and_element.input(connection_id_t {0}), delay_t {10us}, true);
    simulation.submit_event(xor_element.input(connection_id_t {0}), delay_t {10us}, true);

    const auto max_events = 1;
    const auto event_count =
        simulation.run(Simulation::defaults::infinite_simulation_time,
                       Simulation::defaults::no_timeout, max_events);

    EXPECT_EQ(event_count, 2);
}

TEST(SimulationTest, SimulationTimeAdvancingWithoutInfiniteEvents) {
    using namespace std::chrono_literals;

    // create infinite loop
    Schematic schematic;
    auto inverter = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::buffer_element,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .input_inverters = logic_small_vector_t {true},
        .output_delays = {defaults::logic_item_delay},
    });
    auto wire = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .output_delays = {delay_t {100us}},
    });
    inverter.output(connection_id_t {0}).connect(wire.input(connection_id_t {0}));
    wire.output(connection_id_t {0}).connect(inverter.input(connection_id_t {0}));

    auto simulation = get_uninitialized_simulation(schematic);
    simulation.initialize();

    EXPECT_EQ(simulation.time(), time_t {0us});
    simulation.run(delay_t {5ms});
    EXPECT_EQ(simulation.time(), time_t {5ms});
}

TEST(SimulationTest, SimulationInfiniteEventsTimeout) {
    using namespace std::chrono_literals;

    // create infinite loop
    Schematic schematic;
    auto inverter = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::buffer_element,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .input_inverters = logic_small_vector_t {true},
        .output_delays = {defaults::logic_item_delay},
    });
    auto wire = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .output_delays = {delay_t {1us}},
    });
    inverter.output(connection_id_t {0}).connect(wire.input(connection_id_t {0}));
    wire.output(connection_id_t {0}).connect(inverter.input(connection_id_t {0}));

    auto simulation = get_initialized_simulation(schematic);
    // run simulation for 5 ms
    EXPECT_EQ(simulation.time(), time_t {0us});
    const auto start = timeout_clock::now();
    simulation.run(Simulation::defaults::infinite_simulation_time, 5ms);
    const auto end = timeout_clock::now();

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
    auto xor_element = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::xor_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .output_delays = {defaults::logic_item_delay},
    });

    auto simulation = get_initialized_simulation(schematic);
    simulation.run();

    EXPECT_EQ(simulation.input_value(xor_element.input(connection_id_t {0})), false);
    EXPECT_EQ(simulation.input_value(xor_element.input(connection_id_t {1})), false);
    EXPECT_EQ(simulation.output_value(xor_element.output(connection_id_t {0})), false);

    // enable first input
    simulation.submit_event(xor_element.input(connection_id_t {0}), delay_t {10us}, true);
    simulation.run();

    EXPECT_EQ(simulation.input_value(xor_element.input(connection_id_t {0})), true);
    EXPECT_EQ(simulation.input_value(xor_element.input(connection_id_t {1})), false);
    EXPECT_EQ(simulation.output_value(xor_element.output(connection_id_t {0})), true);

    // enable second input
    simulation.submit_event(xor_element.input(connection_id_t {1}), delay_t {10us}, true);
    simulation.run();

    EXPECT_EQ(simulation.input_value(xor_element.input(connection_id_t {0})), true);
    EXPECT_EQ(simulation.input_value(xor_element.input(connection_id_t {1})), true);
    EXPECT_EQ(simulation.output_value(xor_element.output(connection_id_t {0})), false);
}

TEST(SimulationTest, SimulatanousEvents) {
    Schematic schematic;
    auto xor_element = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::xor_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .output_delays = {defaults::logic_item_delay},
    });

    auto simulation = get_initialized_simulation(schematic);
    simulation.submit_event(xor_element.input(connection_id_t {0}), delay_t {10us}, true);
    simulation.run();

    EXPECT_EQ(simulation.input_value(xor_element.input(connection_id_t {0})), true);
    EXPECT_EQ(simulation.input_value(xor_element.input(connection_id_t {1})), false);
    EXPECT_EQ(simulation.output_value(xor_element.output(connection_id_t {0})), true);

    // flip inputs at the same time
    simulation.submit_event(xor_element.input(connection_id_t {0}), delay_t {10us},
                            false);
    simulation.submit_event(xor_element.input(connection_id_t {1}), delay_t {10us}, true);
    simulation.run();

    EXPECT_EQ(simulation.input_value(xor_element.input(connection_id_t {0})), false);
    EXPECT_EQ(simulation.input_value(xor_element.input(connection_id_t {1})), true);
    EXPECT_EQ(simulation.output_value(xor_element.output(connection_id_t {0})), true);
}

TEST(SimulationTest, HalfAdder) {
    Schematic schematic;

    auto input0 = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {2},
        .output_delays = {delay_t {1us}, delay_t {1us}},
    });
    auto input1 = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {2},
        .output_delays = {delay_t {1us}, delay_t {1us}},
    });
    auto carry = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .output_delays = {defaults::logic_item_delay},
    });
    auto output = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::xor_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .output_delays = {defaults::logic_item_delay},
    });

    input0.output(connection_id_t {0}).connect(carry.input(connection_id_t {0}));
    input0.output(connection_id_t {1}).connect(output.input(connection_id_t {0}));

    input1.output(connection_id_t {0}).connect(carry.input(connection_id_t {1}));
    input1.output(connection_id_t {1}).connect(output.input(connection_id_t {1}));

    auto simulation = get_initialized_simulation(schematic);

    // 0 + 0 -> 00
    {
        simulation.submit_event(input0.input(connection_id_t {0}), delay_t {10us}, false);
        simulation.submit_event(input1.input(connection_id_t {0}), delay_t {10us}, false);
        simulation.run();

        EXPECT_EQ(simulation.output_value(output.output(connection_id_t {0})), false);
        EXPECT_EQ(simulation.output_value(carry.output(connection_id_t {0})), false);
    }

    // 0 + 1 = 01
    {
        simulation.submit_event(input0.input(connection_id_t {0}), delay_t {10us}, true);
        simulation.submit_event(input1.input(connection_id_t {0}), delay_t {10us}, false);
        simulation.run();

        EXPECT_EQ(simulation.output_value(output.output(connection_id_t {0})), true);
        EXPECT_EQ(simulation.output_value(carry.output(connection_id_t {0})), false);
    }

    // 1 + 0 = 01
    {
        simulation.submit_event(input0.input(connection_id_t {0}), delay_t {10us}, false);
        simulation.submit_event(input1.input(connection_id_t {0}), delay_t {10us}, true);
        simulation.run();

        EXPECT_EQ(simulation.output_value(output.output(connection_id_t {0})), true);
        EXPECT_EQ(simulation.output_value(carry.output(connection_id_t {0})), false);
    }

    // 1 + 1 = 10
    {
        simulation.submit_event(input0.input(connection_id_t {0}), delay_t {10us}, true);
        simulation.submit_event(input1.input(connection_id_t {0}), delay_t {10us}, true);
        simulation.run();

        EXPECT_EQ(simulation.output_value(output.output(connection_id_t {0})), false);
        EXPECT_EQ(simulation.output_value(carry.output(connection_id_t {0})), true);
    }
}

TEST(SimulationTest, OutputDelayTest) {
    using namespace std::chrono_literals;

    Schematic schematic;
    auto wire = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {3},
        .output_delays = {delay_t {1ms}, delay_t {2ms}, delay_t {3ms}},
    });
    auto simulation = get_initialized_simulation(schematic);

    simulation.submit_event(wire.input(connection_id_t {0}), delay_t {1us}, true);
    simulation.run(delay_t {1us});

    // after 0.5 seconds
    simulation.run(delay_t {500us});
    ASSERT_THAT(simulation.output_values(wire), testing::ElementsAre(0, 0, 0));
    // after 1.5 seconds
    simulation.run(delay_t {1ms});
    ASSERT_THAT(simulation.output_values(wire), testing::ElementsAre(1, 0, 0));
    // after 2.5 seconds
    simulation.run(delay_t {1ms});
    ASSERT_THAT(simulation.output_values(wire), testing::ElementsAre(1, 1, 0));
    // after 3.5 seconds
    simulation.run(delay_t {1ms});
    ASSERT_THAT(simulation.output_values(wire), testing::ElementsAre(1, 1, 1));
}

TEST(SimulationTest, JKFlipFlop) {
    using namespace std::chrono_literals;

    Schematic schematic;
    const auto flipflop = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::flipflop_jk,
        .input_count = connection_count_t {5},
        .output_count = connection_count_t {2},
        .output_delays = {defaults::logic_item_delay, defaults::logic_item_delay},
    });
    auto simulation = get_initialized_simulation(schematic);

    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(0, 1));

    // clk, j, k, set, reset

    // switch to j state
    simulation.submit_events(flipflop, delay_t {1ms}, {true, true, false, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(0, 1));
    simulation.submit_events(flipflop, delay_t {1ms}, {false, true, false, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(1, 0));

    // switch to k state
    simulation.submit_events(flipflop, delay_t {1ms}, {true, false, true, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(1, 0));
    simulation.submit_events(flipflop, delay_t {1ms}, {false, false, true, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(0, 1));

    // toggle state
    simulation.submit_events(flipflop, delay_t {1ms}, {true, true, true, false, false});
    simulation.submit_events(flipflop, delay_t {2ms}, {false, true, true, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(1, 0));
    simulation.submit_events(flipflop, delay_t {1ms}, {true, true, true, false, false});
    simulation.submit_events(flipflop, delay_t {2ms}, {false, true, true, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(0, 1));

    // steady state
    simulation.submit_events(flipflop, delay_t {1ms}, {true, false, false, false, false});
    simulation.submit_events(flipflop, delay_t {2ms},
                             {false, false, false, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(0, 1));

    // TODO test set

    // TODO test reset
}

TEST(SimulationTest, AndInputInverters1) {
    using namespace std::chrono_literals;

    Schematic schematic;
    const auto and_element = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .input_inverters = {true, true},
        .output_delays = {defaults::logic_item_delay},
    });

    auto simulation = get_uninitialized_simulation(schematic);

    simulation.initialize();
    simulation.run();
    ASSERT_THAT(simulation.input_values(and_element), testing::ElementsAre(false, false));
    ASSERT_THAT(simulation.output_values(and_element), testing::ElementsAre(true));

    simulation.submit_event(and_element.input(connection_id_t {0}), delay_t {1ms}, true);
    simulation.run();
    ASSERT_THAT(simulation.input_values(and_element), testing::ElementsAre(true, false));
    ASSERT_THAT(simulation.output_values(and_element), testing::ElementsAre(false));
}

TEST(SimulationTest, AndInputInverters2) {
    using namespace std::chrono_literals;

    Schematic schematic;
    const auto and_element = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .input_inverters = {false, true},
        .output_delays = {defaults::logic_item_delay},
    });

    auto simulation = get_uninitialized_simulation(schematic);

    simulation.initialize();
    simulation.run();
    ASSERT_THAT(simulation.input_values(and_element), testing::ElementsAre(false, false));
    ASSERT_THAT(simulation.output_values(and_element), testing::ElementsAre(false));

    simulation.submit_event(and_element.input(connection_id_t {0}), delay_t {1ms}, true);
    simulation.run();
    ASSERT_THAT(simulation.input_values(and_element), testing::ElementsAre(true, false));
    ASSERT_THAT(simulation.output_values(and_element), testing::ElementsAre(true));
}

TEST(SimulationTest, TestInputHistory) {
    using namespace std::chrono_literals;

    Schematic schematic;
    const auto wire = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {2},
        .output_delays = {delay_t {10us}, delay_t {100us}},
        .history_length = delay_t {100us},
    });

    auto simulation = get_uninitialized_simulation(schematic);

    simulation.initialize();
    simulation.run();
    ASSERT_EQ(simulation.time(), time_t {0us});

    using entry_t = simulation::history_entry_t;
    ASSERT_THAT(simulation.input_history(wire),
                testing::ElementsAre(entry_t {time_t::min(), time_t {0us}, false}));

    simulation.submit_event(wire.input(connection_id_t {0}), delay_t {10us}, true);
    // ignored
    simulation.submit_event(wire.input(connection_id_t {0}), delay_t {20us}, true);
    simulation.submit_event(wire.input(connection_id_t {0}), delay_t {40us}, false);
    simulation.submit_event(wire.input(connection_id_t {0}), delay_t {60us}, true);
    simulation.submit_event(wire.input(connection_id_t {0}), delay_t {180us}, false);

    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.time(), time_t {100us});
    ASSERT_THAT(simulation.input_history(wire),
                testing::ElementsAre(entry_t {time_t::min(), time_t {10us}, false},
                                     entry_t {time_t {10us}, time_t {40us}, true},
                                     entry_t {time_t {40us}, time_t {60us}, false},
                                     entry_t {time_t {60us}, time_t {100us}, true}));

    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.time(), time_t {200us});
    ASSERT_THAT(simulation.input_history(wire),
                testing::ElementsAre(entry_t {time_t::min(), time_t {180us}, true},
                                     entry_t {time_t {180us}, time_t {200us}, false}));
}

TEST(SimulationTest, TestClockGenerator) {
    using namespace std::chrono_literals;

    Schematic schematic;
    auto clock = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::clock_generator,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {3},
        .output_delays = {delay_t {100us}, delay_t {100us}, delay_t {100us}},
    });
    clock.output(connection_id_t {1}).connect(clock.input(connection_id_t {1}));
    clock.output(connection_id_t {2}).connect(clock.input(connection_id_t {2}));

    auto simulation = get_uninitialized_simulation(schematic);

    simulation.initialize();
    simulation.submit_event(clock.input(connection_id_t {0}), delay_t {50us}, true);

    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.output_value(clock.output(connection_id_t {0})), false);
    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.output_value(clock.output(connection_id_t {0})), true);
    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.output_value(clock.output(connection_id_t {0})), false);
    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.output_value(clock.output(connection_id_t {0})), true);
}

TEST(SimulationTest, TestClockGeneratorDifferentDelay) {
    using namespace std::chrono_literals;

    Schematic schematic;
    auto clock = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::clock_generator,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {3},
        .output_delays = {delay_t {100us}, delay_t {500us}, delay_t {500us}},
    });
    clock.output(connection_id_t {1}).connect(clock.input(connection_id_t {1}));
    clock.output(connection_id_t {2}).connect(clock.input(connection_id_t {2}));

    auto simulation = get_uninitialized_simulation(schematic);

    simulation.initialize();
    simulation.submit_event(clock.input(connection_id_t {0}), delay_t {50us}, true);

    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.output_value(clock.output(connection_id_t {0})), false);
    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.output_value(clock.output(connection_id_t {0})), true);
    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.output_value(clock.output(connection_id_t {0})), true);
    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.output_value(clock.output(connection_id_t {0})), true);
    simulation.run(delay_t {100us});
    ASSERT_EQ(simulation.output_value(clock.output(connection_id_t {0})), true);
    simulation.run(delay_t {100us});  // 600 us
    ASSERT_EQ(simulation.output_value(clock.output(connection_id_t {0})), true);
    simulation.run(delay_t {100us});  // 700 us
    ASSERT_EQ(simulation.output_value(clock.output(connection_id_t {0})), false);
}

TEST(SimulationTest, TestClockReset) {
    using namespace std::chrono_literals;

    Schematic schematic;
    auto clock = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::clock_generator,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {3},
        .output_delays = {delay_t {1ns}, delay_t {1ms}, delay_t {1ms}},
    });
    clock.output(connection_id_t {1}).connect(clock.input(connection_id_t {1}));
    clock.output(connection_id_t {2}).connect(clock.input(connection_id_t {2}));

    auto simulation = get_uninitialized_simulation(schematic);

    simulation.initialize();
    simulation.submit_event(clock.input(connection_id_t {0}), delay_t {1000us}, true);
    simulation.submit_event(clock.input(connection_id_t {0}), delay_t {1100us}, false);
    simulation.run(delay_t {10ns});

    ASSERT_EQ(simulation.output_value(clock.output(connection_id_t {0})), false);
    simulation.run(delay_t {1ms});
    ASSERT_EQ(simulation.output_value(clock.output(connection_id_t {0})), true);

    simulation.run(delay_t {999us});
    ASSERT_EQ(simulation.output_value(clock.output(connection_id_t {0})), true);
    simulation.run(delay_t {1us});
    ASSERT_EQ(simulation.output_value(clock.output(connection_id_t {0})), false);

    simulation.run(delay_t {1ms});
    ASSERT_EQ(simulation.output_value(clock.output(connection_id_t {0})), false);
    simulation.run(delay_t {1ms});
    ASSERT_EQ(simulation.output_value(clock.output(connection_id_t {0})), false);
    simulation.run(delay_t {1ms});
    ASSERT_EQ(simulation.output_value(clock.output(connection_id_t {0})), false);
}

TEST(SimulationTest, TestShiftRegister) {
    using namespace std::chrono_literals;

    Schematic schematic;
    auto shift_register = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::shift_register,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {2},
        .output_delays = {defaults::logic_item_delay, defaults::logic_item_delay},
    });

    auto simulation = get_uninitialized_simulation(schematic);
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

//
// History
//

// size

TEST(SimulationTest, HistoryViewSize) {
    auto time = time_t {100us};
    auto history_length = delay_t {7us};
    auto history = simulation::history_buffer_t {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = simulation::HistoryView {history, time, last_value, history_length};

    ASSERT_THAT(view.size(), 2);
}

TEST(SimulationTest, HistoryViewSizeExact) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = simulation::history_buffer_t {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = simulation::HistoryView {history, time, last_value, history_length};

    ASSERT_THAT(view.size(), 2);
}

TEST(SimulationTest, HistoryViewSizeLast) {
    auto time = time_t {100us};
    auto history_length = delay_t {20us};
    auto history = simulation::history_buffer_t {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = simulation::HistoryView {history, time, last_value, history_length};

    ASSERT_THAT(view.size(), 3);
}

TEST(SimulationTest, HistoryViewSizeEmpty) {
    auto time = time_t {10us};
    auto history_length = delay_t {20us};
    auto history = simulation::history_buffer_t {};
    auto last_value = false;

    auto view = simulation::HistoryView {history, time, last_value, history_length};

    ASSERT_THAT(view.size(), 1);
}

TEST(SimulationTest, HistoryViewSizeNegative) {
    auto time = time_t {10us};
    auto history_length = delay_t {20us};
    auto history = simulation::history_buffer_t {time_t {5us}, time_t {7us}};
    auto last_value = false;

    auto view = simulation::HistoryView {history, time, last_value, history_length};

    ASSERT_THAT(view.size(), 3);
}

TEST(SimulationTest, HistoryViewEmpty) {
    const auto view = simulation::HistoryView {};

    ASSERT_THAT(view.size(), 1);
    ASSERT_THAT(view.end() - view.begin(), 1);

    ASSERT_THAT(view.last_value(), false);
    ASSERT_THAT(view.value(time_t {0us}), false);

    const auto value = *view.begin();
    ASSERT_THAT(value.first_time, time_t::min());
    ASSERT_THAT(value.last_time, time_t::max());
    ASSERT_THAT(value.value, false);

    ASSERT_THAT(view.until(time_t {100us}) - view.from(time_t {0us}), 1);
}

// begin end iteration

TEST(SimulationTest, HistoryViewBeginEndExact) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = simulation::history_buffer_t {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = simulation::HistoryView {history, time, last_value, history_length};

    auto begin = view.begin();
    auto end = view.end();

    ASSERT_THAT(view.size(), 2);
    ASSERT_THAT(end - begin, 2);

    ASSERT_THAT(begin == end, false);
    auto value0 = *(begin++);
    ASSERT_THAT(begin == end, false);
    auto value1 = *(begin++);
    ASSERT_THAT(begin == end, true);

    ASSERT_THAT(value0.first_time, time_t::min());
    ASSERT_THAT(value0.last_time, time_t {95us});
    ASSERT_THAT(value0.value, true);

    ASSERT_THAT(value1.first_time, time_t {95us});
    ASSERT_THAT(value1.last_time, time_t {100us});
    ASSERT_THAT(value1.value, false);
}

TEST(SimulationTest, HistoryViewBeginEndFull) {
    auto time = time_t {100us};
    auto history_length = delay_t {50us};
    auto history = simulation::history_buffer_t {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = simulation::HistoryView {history, time, last_value, history_length};

    auto begin = view.begin();
    auto end = view.end();

    ASSERT_THAT(view.size(), 3);
    ASSERT_THAT(end - begin, 3);

    ASSERT_THAT(begin == end, false);
    auto value0 = *(begin++);
    ASSERT_THAT(begin == end, false);
    auto value1 = *(begin++);
    ASSERT_THAT(begin == end, false);
    auto value2 = *(begin++);
    ASSERT_THAT(begin == end, true);

    ASSERT_THAT(value0.first_time, time_t::min());
    ASSERT_THAT(value0.last_time, time_t {90us});
    ASSERT_THAT(value0.value, false);

    ASSERT_THAT(value1.first_time, time_t {90us});
    ASSERT_THAT(value1.last_time, time_t {95us});
    ASSERT_THAT(value1.value, true);

    ASSERT_THAT(value2.first_time, time_t {95us});
    ASSERT_THAT(value2.last_time, time_t {100us});
    ASSERT_THAT(value2.value, false);
}

// before

TEST(SimulationTest, HistoryViewFromExact) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = simulation::history_buffer_t {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = simulation::HistoryView {history, time, last_value, history_length};
    auto from = view.from(time_t {95us});
    ASSERT_THAT(view.end() - from, 1);

    auto value = *from;
    ASSERT_THAT(value.first_time, time_t {95us});
    ASSERT_THAT(value.last_time, time_t {100us});
    ASSERT_THAT(value.value, false);
}

TEST(SimulationTest, HistoryViewFrom) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = simulation::history_buffer_t {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = simulation::HistoryView {history, time, last_value, history_length};
    auto from = view.from(time_t {96us});
    ASSERT_THAT(view.end() - from, 1);

    auto value = *from;
    ASSERT_THAT(value.first_time, time_t {95us});
    ASSERT_THAT(value.last_time, time_t {100us});
    ASSERT_THAT(value.value, false);
}

TEST(SimulationTest, HistoryViewFromSecond) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = simulation::history_buffer_t {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = simulation::HistoryView {history, time, last_value, history_length};
    auto from = view.from(time_t {90us});
    ASSERT_THAT(view.end() - from, 2);

    auto value = *from;
    ASSERT_THAT(value.first_time, time_t::min());
    ASSERT_THAT(value.last_time, time_t {95us});
    ASSERT_THAT(value.value, true);
}

TEST(SimulationTest, HistoryViewFromSmall) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = simulation::history_buffer_t {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = simulation::HistoryView {history, time, last_value, history_length};
    auto from = view.from(time_t {50us});
    ASSERT_THAT(view.end() - from, 2);
}

// until

TEST(SimulationTest, HistoryViewUntil) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = simulation::history_buffer_t {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = simulation::HistoryView {history, time, last_value, history_length};

    auto from = view.from(time_t {90us});
    auto until = view.until(time_t {96us});
    ASSERT_THAT(view.end() - from, 2);
    ASSERT_THAT(until - from, 2);
}

TEST(SimulationTest, HistoryViewUntilExact) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = simulation::history_buffer_t {time_t {90us}, time_t {95us}};
    auto last_value = false;
    constexpr auto epsilon = time_t::epsilon();

    auto view = simulation::HistoryView {history, time, last_value, history_length};

    auto from = view.from(time_t {90us});
    ASSERT_THAT(view.end() - from, 2);

    ASSERT_THAT(view.until(time_t {95us} + epsilon) - from, 2);
    ASSERT_THAT(view.until(time_t {95us}) - from, 1);
}

TEST(SimulationTest, HistoryViewFromUntilBounds) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = simulation::history_buffer_t {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = simulation::HistoryView {history, time, last_value, history_length};

    ASSERT_THAT(view.end() - view.begin(), 2);

    ASSERT_THAT(view.from(time_t::min()) - view.begin(), 0);
    ASSERT_THAT(view.from(time_t {-100us}) - view.begin(), 0);
    ASSERT_THAT(view.from(time_t {0us}) - view.begin(), 0);
    ASSERT_THAT(view.from(time_t {50us}) - view.begin(), 0);
    ASSERT_THAT(view.from(time_t {99us}) - view.begin(), 1);
    ASSERT_THAT(view.from(time_t {100us}) - view.begin(), 1);

    ASSERT_THAT(view.until(time_t::min()) - view.begin(), 1);
    ASSERT_THAT(view.until(time_t {50us}) - view.begin(), 1);
    ASSERT_THAT(view.until(time_t {100us}) - view.begin(), 2);
}

// value

TEST(SimulationTest, HistoryViewValueFull) {
    auto time = time_t {100us};
    auto history_length = delay_t {50us};
    auto history = simulation::history_buffer_t {time_t {90us}, time_t {95us}};
    auto last_value = false;
    constexpr auto epsilon = time_t::epsilon();

    auto view = simulation::HistoryView {history, time, last_value, history_length};

    ASSERT_THAT(view.value(time_t::min()), false);
    ASSERT_THAT(view.value(time_t {-100us}), false);
    ASSERT_THAT(view.value(time_t {0us}), false);

    ASSERT_THAT(view.value(time_t {90us} - epsilon), false);
    ASSERT_THAT(view.value(time_t {90us}), true);

    ASSERT_THAT(view.value(time_t {95us} - epsilon), true);
    ASSERT_THAT(view.value(time_t {95us}), false);

    ASSERT_THAT(view.value(time_t {100us}), false);
}

TEST(SimulationTest, HistoryViewValuePartialHistory) {
    auto time = time_t {100us};
    auto history_length = delay_t {10us};
    auto history = simulation::history_buffer_t {time_t {90us}, time_t {95us}};
    auto last_value = false;
    constexpr auto epsilon = time_t::epsilon();

    auto view = simulation::HistoryView {history, time, last_value, history_length};

    ASSERT_THAT(view.value(time_t::min()), true);
    ASSERT_THAT(view.value(time_t {-100us}), true);
    ASSERT_THAT(view.value(time_t {0us}), true);

    ASSERT_THAT(view.value(time_t {90us} - epsilon), true);
    ASSERT_THAT(view.value(time_t {90us}), true);

    ASSERT_THAT(view.value(time_t {95us} - epsilon), true);
    ASSERT_THAT(view.value(time_t {95us}), false);

    ASSERT_THAT(view.value(time_t {100us}), false);
}

TEST(SimulationTest, HistoryViewIteratorValues) {
    const auto time = time_t {100us};
    const auto history_length = delay_t {100us};
    const auto history = simulation::history_buffer_t {time_t {90us}, time_t {95us}};
    const auto last_value = false;

    const auto view = simulation::HistoryView {history, time, last_value, history_length};

    {
        auto it = view.from(time_t {95us});
        const auto end = view.until(time_t {100us});

        ASSERT_THAT((*it).first_time, time_t {95us});
        ASSERT_THAT((*it).last_time, time_t {100us});
        ASSERT_THAT((*it).value, false);

        ASSERT_THAT(end - it, 1);
        it++;
        ASSERT_THAT(it == end, true);
    }

    {
        auto it = view.from(time_t {92us});
        const auto end = view.until(time_t {95us});

        ASSERT_THAT((*it).first_time, time_t {90us});
        ASSERT_THAT((*it).last_time, time_t {95us});
        ASSERT_THAT((*it).value, true);

        ASSERT_THAT(end - it, 1);
        it++;
        ASSERT_THAT(it == end, true);
    }
}

}  // namespace logicsim
