
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

// Simulation

[[nodiscard]] auto get_uninitialized_simulation(Schematic&& schematic) -> Simulation {
    add_missing_placeholders(schematic);

    return Simulation {std::move(schematic)};
}

[[nodiscard]] auto get_initialized_simulation(Schematic&& schematic) -> Simulation {
    add_missing_placeholders(schematic);

    Simulation simulation {std::move(schematic)};
    simulation.initialize();
    return simulation;
}

TEST(SimulationTest, InitializeSimulation) {
    Schematic schematic;
    auto inverter = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::buffer_element,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .input_inverters = logic_small_vector_t {true},
        .output_delays = {element_output_delay(ElementType::buffer_element)},
    });

    auto simulation = get_initialized_simulation(std::move(schematic));
    simulation.run();

    EXPECT_EQ(simulation.input_value(input_t {inverter, connection_id_t {0}}), false);
    EXPECT_EQ(simulation.output_value(output_t {inverter, connection_id_t {0}}), true);
}

TEST(SimulationTest, SimulationTimeAdvancingWithoutEvents) {
    using namespace std::chrono_literals;

    Schematic schematic;
    auto simulation = get_initialized_simulation(std::move(schematic));

    EXPECT_EQ(simulation.time(), time_t {0us});
    simulation.run(delay_t {3s});
    EXPECT_EQ(simulation.time(), time_t {3s});
}

TEST(SimulationTest, SimulationProcessAllEventsForTime) {
    using namespace std::chrono_literals;

    Schematic schematic;
    auto and_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false},
        .output_delays = {element_output_delay(ElementType::and_element)},
    });
    auto xor_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::xor_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false},
        .output_delays = {element_output_delay(ElementType::xor_element)},
    });
    auto simulation = get_initialized_simulation(std::move(schematic));

    const auto id_0 = connection_id_t {0};

    simulation.submit_event(input_t {and_element, id_0}, delay_t {10us}, true);
    simulation.submit_event(input_t {xor_element, id_0}, delay_t {10us}, true);

    const auto max_events = 1;
    const auto event_count =
        simulation.run(simulation::defaults::infinite_simulation_time,
                       simulation::defaults::no_realtime_timeout, max_events);

    EXPECT_EQ(event_count, 2);
}

TEST(SimulationTest, SimulationTimeAdvancingWithoutInfiniteEvents) {
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
        .output_delays = {delay_t {100us}},
    });

    const auto id_0 = connection_id_t {0};
    schematic.connect(output_t {inverter, id_0}, input_t {wire, id_0});
    schematic.connect(output_t {wire, id_0}, input_t {inverter, id_0});

    auto simulation = get_uninitialized_simulation(std::move(schematic));
    simulation.initialize();

    EXPECT_EQ(simulation.time(), time_t {0us});
    simulation.run(delay_t {5ms});
    EXPECT_EQ(simulation.time(), time_t {5ms});
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

    auto simulation = get_initialized_simulation(std::move(schematic));
    // run simulation for 5 ms
    EXPECT_EQ(simulation.time(), time_t {0us});
    const auto start = std::chrono::steady_clock::now();
    simulation.run(simulation::defaults::infinite_simulation_time, 5ms);
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

    auto simulation = get_initialized_simulation(std::move(schematic));
    simulation.run();

    const auto input_0 = input_t {xor_element, connection_id_t {0}};
    const auto input_1 = input_t {xor_element, connection_id_t {1}};
    const auto output_0 = output_t {xor_element, connection_id_t {0}};

    EXPECT_EQ(simulation.input_value(input_0), false);
    EXPECT_EQ(simulation.input_value(input_1), false);
    EXPECT_EQ(simulation.output_value(output_0), false);

    // enable first input
    simulation.submit_event(input_0, delay_t {10us}, true);
    simulation.run();

    EXPECT_EQ(simulation.input_value(input_0), true);
    EXPECT_EQ(simulation.input_value(input_1), false);
    EXPECT_EQ(simulation.output_value(output_0), true);

    // enable second input
    simulation.submit_event(input_1, delay_t {10us}, true);
    simulation.run();

    EXPECT_EQ(simulation.input_value(input_0), true);
    EXPECT_EQ(simulation.input_value(input_1), true);
    EXPECT_EQ(simulation.output_value(output_0), false);
}

TEST(SimulationTest, SimulatanousEvents) {
    Schematic schematic;
    auto xor_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::xor_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false},
        .output_delays = {element_output_delay(ElementType::xor_element)},
    });

    const auto input_0 = input_t {xor_element, connection_id_t {0}};
    const auto input_1 = input_t {xor_element, connection_id_t {1}};
    const auto output_0 = output_t {xor_element, connection_id_t {0}};

    auto simulation = get_initialized_simulation(std::move(schematic));
    simulation.submit_event(input_0, delay_t {10us}, true);
    simulation.run();

    EXPECT_EQ(simulation.input_value(input_0), true);
    EXPECT_EQ(simulation.input_value(input_1), false);
    EXPECT_EQ(simulation.output_value(output_0), true);

    // flip inputs at the same time
    simulation.submit_event(input_0, delay_t {10us}, false);
    simulation.submit_event(input_1, delay_t {10us}, true);
    simulation.run();

    EXPECT_EQ(simulation.input_value(input_0), false);
    EXPECT_EQ(simulation.input_value(input_1), true);
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

    auto simulation = get_initialized_simulation(std::move(schematic));

    // 0 + 0 -> 00
    {
        simulation.submit_event(input_t {input0, id_0}, delay_t {10us}, false);
        simulation.submit_event(input_t {input1, id_0}, delay_t {10us}, false);
        simulation.run();

        EXPECT_EQ(simulation.output_value(output_t {output, id_0}), false);
        EXPECT_EQ(simulation.output_value(output_t {carry, id_0}), false);
    }

    // 0 + 1 = 01
    {
        simulation.submit_event(input_t {input0, id_0}, delay_t {10us}, true);
        simulation.submit_event(input_t {input1, id_0}, delay_t {10us}, false);
        simulation.run();

        EXPECT_EQ(simulation.output_value(output_t {output, id_0}), true);
        EXPECT_EQ(simulation.output_value(output_t {carry, id_0}), false);
    }

    // 1 + 0 = 01
    {
        simulation.submit_event(input_t {input0, id_0}, delay_t {10us}, false);
        simulation.submit_event(input_t {input1, id_0}, delay_t {10us}, true);
        simulation.run();

        EXPECT_EQ(simulation.output_value(output_t {output, id_0}), true);
        EXPECT_EQ(simulation.output_value(output_t {carry, id_0}), false);
    }

    // 1 + 1 = 10
    {
        simulation.submit_event(input_t {input0, id_0}, delay_t {10us}, true);
        simulation.submit_event(input_t {input1, id_0}, delay_t {10us}, true);
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

    auto simulation = get_initialized_simulation(std::move(schematic));

    simulation.submit_event(input_t {wire, id_0}, delay_t {1us}, true);
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
    const auto flipflop = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::flipflop_jk,
        .input_count = connection_count_t {5},
        .output_count = connection_count_t {2},
        .input_inverters = {false, false, false, false, false},
        .output_delays = {element_output_delay(ElementType::flipflop_jk),
                          element_output_delay(ElementType::flipflop_jk)},
    });
    auto simulation = get_initialized_simulation(std::move(schematic));

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
    const auto and_element = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .input_inverters = {true, true},
        .output_delays = {element_output_delay(ElementType::and_element)},
    });

    auto simulation = get_uninitialized_simulation(std::move(schematic));

    const auto id_0 = connection_id_t {0};

    simulation.initialize();
    simulation.run();
    ASSERT_THAT(simulation.input_values(and_element), testing::ElementsAre(false, false));
    ASSERT_THAT(simulation.output_values(and_element), testing::ElementsAre(true));

    simulation.submit_event(input_t {and_element, id_0}, delay_t {1ms}, true);
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

    auto simulation = get_uninitialized_simulation(std::move(schematic));

    const auto id_0 = connection_id_t {0};

    simulation.initialize();
    simulation.run();
    ASSERT_THAT(simulation.input_values(and_element), testing::ElementsAre(false, false));
    ASSERT_THAT(simulation.output_values(and_element), testing::ElementsAre(false));

    simulation.submit_event(input_t {and_element, id_0}, delay_t {1ms}, true);
    simulation.run();
    ASSERT_THAT(simulation.input_values(and_element), testing::ElementsAre(true, false));
    ASSERT_THAT(simulation.output_values(and_element), testing::ElementsAre(true));
}

TEST(SimulationTest, TestInputHistory) {
    using namespace std::chrono_literals;

    Schematic schematic;
    const auto wire = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {2},
        .input_inverters = {false},
        .output_delays = {delay_t {10us}, delay_t {100us}},
        .history_length = delay_t {100us},
    });

    auto simulation = get_uninitialized_simulation(std::move(schematic));

    const auto wire_0 = input_t {wire, connection_id_t {0}};

    simulation.initialize();
    simulation.run();
    ASSERT_EQ(simulation.time(), time_t {0us});

    using entry_t = simulation::history_entry_t;
    ASSERT_THAT(simulation.input_history(wire),
                testing::ElementsAre(entry_t {time_t::min(), time_t {0us}, false}));

    simulation.submit_event(wire_0, delay_t {10us}, true);
    // ignored
    simulation.submit_event(wire_0, delay_t {20us}, true);
    simulation.submit_event(wire_0, delay_t {40us}, false);
    simulation.submit_event(wire_0, delay_t {60us}, true);
    simulation.submit_event(wire_0, delay_t {180us}, false);

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

    auto simulation = get_uninitialized_simulation(std::move(schematic));

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

    auto simulation = get_uninitialized_simulation(std::move(schematic));

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

    auto simulation = get_uninitialized_simulation(std::move(schematic));

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

    auto simulation = get_uninitialized_simulation(std::move(schematic));
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

}  // namespace logicsim
