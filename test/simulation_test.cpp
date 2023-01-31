
#include "simulation.h"

#include "format.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>

namespace logicsim {

// SimulationEvent

TEST(SimulationEventTest, EqualOperatorTest) {
    SimulationEvent event1 {123us, 1, 2, true};
    SimulationEvent event2 {123us, 1, 2, true};
    EXPECT_TRUE(event1 == event2);

    SimulationEvent event3 {123us, 1, 3, true};
    SimulationEvent event4 {123us, 1, 2, false};
    EXPECT_TRUE(event3 == event4);
}

TEST(SimulationEventTest, LessThanOperatorTest) {
    SimulationEvent event1 {123us, 1, 2, true};
    SimulationEvent event2 {789us, 3, 4, false};
    EXPECT_TRUE(event1 < event2);

    SimulationEvent event3 {123us, 1, 4, true};
    SimulationEvent event4 {123us, 3, 2, false};
    EXPECT_TRUE(event3 < event4);
}

TEST(SimulationEventTest, NotEqualOperatorTest) {
    SimulationEvent event1 {123ns, 1, 2, true};
    SimulationEvent event2 {789ns, 3, 4, false};
    EXPECT_TRUE(event1 != event2);
}

TEST(SimulationEventTest, GreaterThanOperatorTest) {
    SimulationEvent event1 {123ns, 1, 2, true};
    SimulationEvent event2 {789ns, 3, 4, false};
    EXPECT_TRUE(event2 > event1);
}

TEST(SimulationEventTest, LessThanOrEqualOperatorTest) {
    SimulationEvent event1 {123ns, 1, 2, true};
    SimulationEvent event2 {789ns, 3, 4, false};
    EXPECT_TRUE(event1 <= event2);
}

TEST(SimulationEventTest, GreaterThanOrEqualOperatorTest) {
    SimulationEvent event1 {123ns, 1, 2, true};
    SimulationEvent event2 {789ns, 3, 4, false};
    EXPECT_TRUE(event2 >= event1);
}

// Simulation

[[nodiscard]] auto get_uninitialized_simulation(Circuit &circuit) -> Simulation {
    add_output_placeholders(circuit);
    circuit.validate(true);

    return Simulation {circuit};
}

[[nodiscard]] auto get_initialized_simulation(Circuit &circuit) -> Simulation {
    add_output_placeholders(circuit);
    circuit.validate(true);

    Simulation simulation {circuit};
    simulation.initialize();
    return simulation;
}

TEST(SimulationTest, InitializeSimulation) {
    Circuit circuit;
    auto inverter {circuit.add_element(ElementType::inverter_element, 1, 1)};

    auto simulation = get_initialized_simulation(circuit);
    simulation.run();

    EXPECT_EQ(simulation.input_value(inverter.input(0)), false);
    EXPECT_EQ(simulation.output_value(inverter.output(0)), true);
}

TEST(SimulationTest, SimulationTimeAdvancingWithoutEvents) {
    using namespace std::chrono_literals;
    Circuit circuit;

    auto simulation = get_initialized_simulation(circuit);

    EXPECT_EQ(simulation.time(), 0us);
    simulation.run(3s);
    EXPECT_EQ(simulation.time(), 3s);
}

TEST(SimulationTest, SimulationTimeAdvancingWithoutInfiniteEvents) {
    using namespace std::chrono_literals;

    // create infinite loop
    Circuit circuit;
    const auto inverter = circuit.add_element(ElementType::inverter_element, 1, 1);
    inverter.output(0).connect(inverter.input(0));

    auto simulation = get_uninitialized_simulation(circuit);
    simulation.set_output_delay(inverter.output(0), delay_t {100us});
    simulation.initialize();

    EXPECT_EQ(simulation.time(), 0us);
    simulation.run(5ms);
    EXPECT_EQ(simulation.time(), 5ms);
}

TEST(SimulationTest, SimulationInfiniteEventsTimeout) {
    using namespace std::chrono_literals;

    // create infinite loop
    Circuit circuit;
    const auto inverter = circuit.add_element(ElementType::inverter_element, 1, 1);
    inverter.output(0).connect(inverter.input(0));
    auto simulation = get_initialized_simulation(circuit);

    // run simulation for 5 ms
    EXPECT_EQ(simulation.time(), 0us);
    const auto start = timeout_clock::now();
    simulation.run(Simulation::defaults::infinite_simulation_time, 5ms);
    const auto end = timeout_clock::now();

    EXPECT_GT(simulation.time(), 1ms);
    const auto delay = end - start;
    ASSERT_THAT(delay > 4ms, true);
    ASSERT_THAT(delay < 6ms, true);
}

TEST(SimulationTest, AdditionalEvents) {
    Circuit circuit;
    auto xor_element {circuit.add_element(ElementType::xor_element, 2, 1)};

    auto simulation = get_initialized_simulation(circuit);
    simulation.run();

    EXPECT_EQ(simulation.input_value(xor_element.input(0)), false);
    EXPECT_EQ(simulation.input_value(xor_element.input(1)), false);
    EXPECT_EQ(simulation.output_value(xor_element.output(0)), false);

    // enable first input
    simulation.submit_event(xor_element.input(0), 10us, true);
    simulation.run();

    EXPECT_EQ(simulation.input_value(xor_element.input(0)), true);
    EXPECT_EQ(simulation.input_value(xor_element.input(1)), false);
    EXPECT_EQ(simulation.output_value(xor_element.output(0)), true);

    // enable second input
    simulation.submit_event(xor_element.input(1), 10us, true);
    simulation.run();

    EXPECT_EQ(simulation.input_value(xor_element.input(0)), true);
    EXPECT_EQ(simulation.input_value(xor_element.input(1)), true);
    EXPECT_EQ(simulation.output_value(xor_element.output(0)), false);
}

TEST(SimulationTest, SimulatanousEvents) {
    Circuit circuit;
    auto xor_element {circuit.add_element(ElementType::xor_element, 2, 1)};

    auto simulation = get_initialized_simulation(circuit);
    simulation.submit_event(xor_element.input(0), 10us, true);
    simulation.run();

    EXPECT_EQ(simulation.input_value(xor_element.input(0)), true);
    EXPECT_EQ(simulation.input_value(xor_element.input(1)), false);
    EXPECT_EQ(simulation.output_value(xor_element.output(0)), true);

    // flip inputs at the same time
    simulation.submit_event(xor_element.input(0), 10us, false);
    simulation.submit_event(xor_element.input(1), 10us, true);
    simulation.run();

    EXPECT_EQ(simulation.input_value(xor_element.input(0)), false);
    EXPECT_EQ(simulation.input_value(xor_element.input(1)), true);
    EXPECT_EQ(simulation.output_value(xor_element.output(0)), true);
}

TEST(SimulationTest, HalfAdder) {
    Circuit circuit;
    const auto input0 {circuit.add_element(ElementType::wire, 1, 2)};
    const auto input1 {circuit.add_element(ElementType::wire, 1, 2)};
    const auto carry {circuit.add_element(ElementType::and_element, 2, 1)};
    const auto output {circuit.add_element(ElementType::xor_element, 2, 1)};

    input0.output(0).connect(carry.input(0));
    input0.output(1).connect(output.input(0));

    input1.output(0).connect(carry.input(1));
    input1.output(1).connect(output.input(1));

    auto simulation = get_initialized_simulation(circuit);

    // 0 + 0 -> 00
    {
        simulation.submit_event(input0.input(0), 10us, false);
        simulation.submit_event(input1.input(0), 10us, false);
        simulation.run();

        EXPECT_EQ(simulation.output_value(output.output(0)), false);
        EXPECT_EQ(simulation.output_value(carry.output(0)), false);
    }

    // 0 + 1 = 01
    {
        simulation.submit_event(input0.input(0), 10us, true);
        simulation.submit_event(input1.input(0), 10us, false);
        simulation.run();

        EXPECT_EQ(simulation.output_value(output.output(0)), true);
        EXPECT_EQ(simulation.output_value(carry.output(0)), false);
    }

    // 1 + 0 = 01
    {
        simulation.submit_event(input0.input(0), 10us, false);
        simulation.submit_event(input1.input(0), 10us, true);
        simulation.run();

        EXPECT_EQ(simulation.output_value(output.output(0)), true);
        EXPECT_EQ(simulation.output_value(carry.output(0)), false);
    }

    // 1 + 1 = 10
    {
        simulation.submit_event(input0.input(0), 10us, true);
        simulation.submit_event(input1.input(0), 10us, true);
        simulation.run();

        EXPECT_EQ(simulation.output_value(output.output(0)), false);
        EXPECT_EQ(simulation.output_value(carry.output(0)), true);
    }
}

TEST(SimulationTest, OutputDelayTest) {
    using namespace std::chrono_literals;

    Circuit circuit;
    const auto wire = circuit.add_element(ElementType::wire, 1, 3);
    auto simulation = get_initialized_simulation(circuit);

    simulation.set_output_delay(wire.output(0), delay_t {1ms});
    simulation.set_output_delay(wire.output(1), delay_t {2ms});
    simulation.set_output_delay(wire.output(2), delay_t {3ms});

    simulation.submit_event(wire.input(0), 1us, true);
    simulation.run(1us);

    // after 0.5 seconds
    simulation.run(500us);
    ASSERT_THAT(simulation.output_values(wire), testing::ElementsAre(0, 0, 0));
    // after 1.5 seconds
    simulation.run(1ms);
    ASSERT_THAT(simulation.output_values(wire), testing::ElementsAre(1, 0, 0));
    // after 2.5 seconds
    simulation.run(1ms);
    ASSERT_THAT(simulation.output_values(wire), testing::ElementsAre(1, 1, 0));
    // after 3.5 seconds
    simulation.run(1ms);
    ASSERT_THAT(simulation.output_values(wire), testing::ElementsAre(1, 1, 1));
}

TEST(SimulationTest, JKFlipFlop) {
    using namespace std::chrono_literals;

    Circuit circuit;
    const auto flipflop {circuit.add_element(ElementType::flipflop_jk, 3, 2)};
    auto simulation = get_initialized_simulation(circuit);

    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(0, 1));

    // switch to j state
    simulation.submit_events(flipflop, 1ms, {true, true, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(1, 0));
    simulation.submit_events(flipflop, 1ms, {false, true, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(1, 0));

    // switch to k state
    simulation.submit_events(flipflop, 1ms, {true, false, true});
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(0, 1));
    simulation.submit_events(flipflop, 1ms, {false, false, true});
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(0, 1));

    // toggle state
    simulation.submit_events(flipflop, 1ms, {true, true, true});
    simulation.submit_events(flipflop, 2ms, {false, true, true});
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(1, 0));
    simulation.submit_events(flipflop, 1ms, {true, true, true});
    simulation.submit_events(flipflop, 2ms, {false, true, true});
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(0, 1));

    // steady state
    simulation.submit_events(flipflop, 1ms, {true, false, false});
    simulation.submit_events(flipflop, 2ms, {false, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(flipflop), testing::ElementsAre(0, 1));
}

TEST(SimulationTest, AndInputInverters) {
    using namespace std::chrono_literals;

    Circuit circuit;
    auto and_element {circuit.add_element(ElementType::and_element, 2, 1)};

    auto simulation = get_uninitialized_simulation(circuit);
    simulation.set_input_inverters(and_element, {true, true});

    simulation.initialize();
    simulation.run();
    ASSERT_THAT(simulation.input_values(and_element), testing::ElementsAre(false, false));
    ASSERT_THAT(simulation.output_values(and_element), testing::ElementsAre(true));

    simulation.set_input_inverters(and_element, {false, true});
    simulation.initialize();
    simulation.run();
    ASSERT_THAT(simulation.input_values(and_element), testing::ElementsAre(false, false));
    ASSERT_THAT(simulation.output_values(and_element), testing::ElementsAre(false));

    simulation.submit_event(and_element.input(0), 1ms, true);
    simulation.run();
    ASSERT_THAT(simulation.input_values(and_element), testing::ElementsAre(true, false));
    ASSERT_THAT(simulation.output_values(and_element), testing::ElementsAre(true));
}

TEST(SimulationTest, TestInputHistory) {
    using namespace std::chrono_literals;

    Circuit circuit;
    auto wire {circuit.add_element(ElementType::wire, 1, 2)};

    auto simulation = get_uninitialized_simulation(circuit);
    simulation.set_max_history(wire, history_t {100us});

    simulation.initialize();
    simulation.run();
    ASSERT_EQ(simulation.time(), time_t {0us});

    using entry_t = Simulation::history_entry_t;
    constexpr auto min_rep = ++time_t::zero();
    ASSERT_THAT(simulation.input_history(wire),
                testing::ElementsAre(entry_t {time_t::min(), 0us, false}));

    simulation.submit_event(wire.input(0), 10us, true);
    simulation.submit_event(wire.input(0), 20us, true);  // shall be ignored
    simulation.submit_event(wire.input(0), 40us, false);
    simulation.submit_event(wire.input(0), 60us, true);
    simulation.submit_event(wire.input(0), 180us, false);

    simulation.run(time_t {100us});
    ASSERT_EQ(simulation.time(), time_t {100us});
    ASSERT_THAT(simulation.input_history(wire),
                testing::ElementsAre(entry_t {time_t::min(), 10us - min_rep, false},
                                     entry_t {10us, 40us - min_rep, true},
                                     entry_t {40us, 60us - min_rep, false},
                                     entry_t {60us, 100us, true}));

    simulation.run(time_t {100us});
    ASSERT_EQ(simulation.time(), time_t {200us});
    ASSERT_THAT(simulation.input_history(wire),
                testing::ElementsAre(entry_t {time_t::min(), 180us - min_rep, true},
                                     entry_t {180us, 200us, false}));
}

TEST(SimulationTest, TestClockGenerator) {
    using namespace std::chrono_literals;

    Circuit circuit;
    auto clock {circuit.add_element(ElementType::clock_generator, 2, 2)};
    clock.output(0).connect(clock.input(0));

    auto simulation = get_uninitialized_simulation(circuit);

    simulation.set_output_delay(clock.output(0), delay_t {100us});
    simulation.set_output_delay(clock.output(1), delay_t {100us});

    simulation.initialize();
    simulation.submit_event(clock.input(1), 50us, true);

    simulation.run(100us);
    ASSERT_EQ(simulation.output_value(clock.output(1)), false);
    simulation.run(100us);
    ASSERT_EQ(simulation.output_value(clock.output(1)), true);
    simulation.run(100us);
    ASSERT_EQ(simulation.output_value(clock.output(1)), false);
    simulation.run(100us);
    ASSERT_EQ(simulation.output_value(clock.output(1)), true);
}

TEST(SimulationTest, TestClockGeneratorDifferentDelay) {
    using namespace std::chrono_literals;

    Circuit circuit;
    auto clock {circuit.add_element(ElementType::clock_generator, 2, 2)};
    clock.output(0).connect(clock.input(0));

    auto simulation = get_uninitialized_simulation(circuit);
    simulation.set_output_delay(clock.output(0), delay_t {500us});
    simulation.set_output_delay(clock.output(1), delay_t {100us});

    simulation.initialize();
    simulation.submit_event(clock.input(1), 50us, true);

    simulation.run(100us);
    ASSERT_EQ(simulation.output_value(clock.output(1)), false);
    simulation.run(100us);
    ASSERT_EQ(simulation.output_value(clock.output(1)), true);
    simulation.run(100us);
    ASSERT_EQ(simulation.output_value(clock.output(1)), true);
    simulation.run(100us);
    ASSERT_EQ(simulation.output_value(clock.output(1)), true);
    simulation.run(100us);
    ASSERT_EQ(simulation.output_value(clock.output(1)), true);
    simulation.run(100us);  // 600 us
    ASSERT_EQ(simulation.output_value(clock.output(1)), true);
    simulation.run(100us);  // 700 us
    ASSERT_EQ(simulation.output_value(clock.output(1)), false);
}

TEST(SimulationTest, TestClockReset) {
    using namespace std::chrono_literals;

    Circuit circuit;
    auto clock {circuit.add_element(ElementType::clock_generator, 2, 2)};
    clock.output(0).connect(clock.input(0));

    auto simulation = get_uninitialized_simulation(circuit);
    simulation.set_output_delay(clock.output(0), delay_t {1ms});
    simulation.set_output_delay(clock.output(1), delay_t {1ns});

    simulation.initialize();
    simulation.submit_event(clock.input(1), 1000us, true);
    simulation.submit_event(clock.input(1), 1100us, false);
    simulation.run(10ns);

    ASSERT_EQ(simulation.output_value(clock.output(1)), false);
    simulation.run(1ms);
    ASSERT_EQ(simulation.output_value(clock.output(1)), true);

    simulation.run(999us);
    ASSERT_EQ(simulation.output_value(clock.output(1)), true);
    simulation.run(1us);
    ASSERT_EQ(simulation.output_value(clock.output(1)), false);

    simulation.run(1ms);
    ASSERT_EQ(simulation.output_value(clock.output(1)), false);
    simulation.run(1ms);
    ASSERT_EQ(simulation.output_value(clock.output(1)), false);
    simulation.run(1ms);
    ASSERT_EQ(simulation.output_value(clock.output(1)), false);
}

TEST(SimulationTest, TestShiftRegister) {
    using namespace std::chrono_literals;

    Circuit circuit;
    auto shift_register {circuit.add_element(ElementType::shift_register, 3, 2)};

    auto simulation = get_uninitialized_simulation(circuit);
    simulation.initialize();

    simulation.run();
    // initial state
    ASSERT_THAT(simulation.output_values(shift_register), testing::ElementsAre(0, 0));
    ASSERT_THAT(simulation.internal_state(shift_register),
                testing::ElementsAre(0, 0, 0, 0, 0, 0, 0, 0));

    // insert first element
    simulation.submit_events(shift_register, 1ms, {true, true, false});
    simulation.submit_events(shift_register, 2ms, {false, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(shift_register), testing::ElementsAre(0, 0));
    ASSERT_THAT(simulation.internal_state(shift_register),
                testing::ElementsAre(1, 0, 0, 0, 0, 0, 0, 0));

    // insert second element
    simulation.submit_events(shift_register, 1ms, {true, false, true});
    simulation.submit_events(shift_register, 2ms, {false, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(shift_register), testing::ElementsAre(0, 0));
    ASSERT_THAT(simulation.internal_state(shift_register),
                testing::ElementsAre(0, 1, 1, 0, 0, 0, 0, 0));

    // insert third element
    simulation.submit_events(shift_register, 1ms, {true, true, true});
    simulation.submit_events(shift_register, 2ms, {false, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(shift_register), testing::ElementsAre(0, 0));
    ASSERT_THAT(simulation.internal_state(shift_register),
                testing::ElementsAre(1, 1, 0, 1, 1, 0, 0, 0));

    // insert forth element  &  receive first element
    simulation.submit_events(shift_register, 1ms, {true, false, false});
    simulation.submit_events(shift_register, 2ms, {false, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(shift_register), testing::ElementsAre(1, 0));
    ASSERT_THAT(simulation.internal_state(shift_register),
                testing::ElementsAre(0, 0, 1, 1, 0, 1, 1, 0));

    // receive second element
    simulation.submit_events(shift_register, 1ms, {true, false, false});
    simulation.submit_events(shift_register, 2ms, {false, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(shift_register), testing::ElementsAre(0, 1));
    ASSERT_THAT(simulation.internal_state(shift_register),
                testing::ElementsAre(0, 0, 0, 0, 1, 1, 0, 1));

    // receive third element
    simulation.submit_events(shift_register, 1ms, {true, false, false});
    simulation.submit_events(shift_register, 2ms, {false, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(shift_register), testing::ElementsAre(1, 1));
    ASSERT_THAT(simulation.internal_state(shift_register),
                testing::ElementsAre(0, 0, 0, 0, 0, 0, 1, 1));

    // receive fourth element
    simulation.submit_events(shift_register, 1ms, {true, false, false});
    simulation.submit_events(shift_register, 2ms, {false, false, false});
    simulation.run();
    ASSERT_THAT(simulation.output_values(shift_register), testing::ElementsAre(0, 0));
    ASSERT_THAT(simulation.internal_state(shift_register),
                testing::ElementsAre(0, 0, 0, 0, 0, 0, 0, 0));
}

//
// History
//

// size

TEST(SimulationTest, HistoryViewSize) {
    auto time = time_t {100us};
    auto max_history = history_t {7us};
    auto history = Simulation::history_vector_t {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = Simulation::HistoryView {history, time, last_value, max_history};

    ASSERT_THAT(view.size(), 2);
}

TEST(SimulationTest, HistoryViewSizeExact) {
    auto time = time_t {100us};
    auto max_history = history_t {10us};
    auto history = Simulation::history_vector_t {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = Simulation::HistoryView {history, time, last_value, max_history};

    ASSERT_THAT(view.size(), 2);
}

TEST(SimulationTest, HistoryViewSizeLast) {
    auto time = time_t {100us};
    auto max_history = history_t {20us};
    auto history = Simulation::history_vector_t {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = Simulation::HistoryView {history, time, last_value, max_history};

    ASSERT_THAT(view.size(), 3);
}

TEST(SimulationTest, HistoryViewSizeEmpty) {
    auto time = time_t {10us};
    auto max_history = history_t {20us};
    auto history = Simulation::history_vector_t {};
    auto last_value = false;

    auto view = Simulation::HistoryView {history, time, last_value, max_history};

    ASSERT_THAT(view.size(), 1);
}

TEST(SimulationTest, HistoryViewSizeNegative) {
    auto time = time_t {10us};
    auto max_history = history_t {20us};
    auto history = Simulation::history_vector_t {5us, 7us};
    auto last_value = false;

    auto view = Simulation::HistoryView {history, time, last_value, max_history};

    ASSERT_THAT(view.size(), 3);
}

// begin end iteration

TEST(SimulationTest, HistoryViewBeginEndExact) {
    auto time = time_t {100us};
    auto max_history = history_t {10us};
    auto history = Simulation::history_vector_t {time_t {90us}, time_t {95us}};
    auto last_value = false;
    constexpr auto min_rep = ++time_t::zero();

    auto view = Simulation::HistoryView {history, time, last_value, max_history};

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
    ASSERT_THAT(value0.last_time, time_t {95us} - min_rep);
    ASSERT_THAT(value0.value, true);

    ASSERT_THAT(value1.first_time, time_t {95us});
    ASSERT_THAT(value1.last_time, time_t {100us});
    ASSERT_THAT(value1.value, false);
}

TEST(SimulationTest, HistoryViewBeginEndFull) {
    auto time = time_t {100us};
    auto max_history = history_t {50us};
    auto history = Simulation::history_vector_t {time_t {90us}, time_t {95us}};
    auto last_value = false;
    constexpr auto min_rep = ++time_t::zero();

    auto view = Simulation::HistoryView {history, time, last_value, max_history};

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
    ASSERT_THAT(value0.last_time, time_t {90us} - min_rep);
    ASSERT_THAT(value0.value, false);

    ASSERT_THAT(value1.first_time, time_t {90us});
    ASSERT_THAT(value1.last_time, time_t {95us} - min_rep);
    ASSERT_THAT(value1.value, true);

    ASSERT_THAT(value2.first_time, time_t {95us});
    ASSERT_THAT(value2.last_time, time_t {100us});
    ASSERT_THAT(value2.value, false);
}

// before

TEST(SimulationTest, HistoryViewFromExact) {
    auto time = time_t {100us};
    auto max_history = history_t {10us};
    auto history = Simulation::history_vector_t {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = Simulation::HistoryView {history, time, last_value, max_history};
    auto from = view.from(time_t {95us});
    ASSERT_THAT(view.end() - from, 1);

    auto value = *from;
    ASSERT_THAT(value.first_time, time_t {95us});
    ASSERT_THAT(value.last_time, time_t {100us});
    ASSERT_THAT(value.value, false);
}

TEST(SimulationTest, HistoryViewFrom) {
    auto time = time_t {100us};
    auto max_history = history_t {10us};
    auto history = Simulation::history_vector_t {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = Simulation::HistoryView {history, time, last_value, max_history};
    auto from = view.from(time_t {96us});
    ASSERT_THAT(view.end() - from, 1);

    auto value = *from;
    ASSERT_THAT(value.first_time, time_t {95us});
    ASSERT_THAT(value.last_time, time_t {100us});
    ASSERT_THAT(value.value, false);
}

TEST(SimulationTest, HistoryViewFromSecond) {
    auto time = time_t {100us};
    auto max_history = history_t {10us};
    auto history = Simulation::history_vector_t {time_t {90us}, time_t {95us}};
    auto last_value = false;
    constexpr auto min_rep = ++time_t::zero();

    auto view = Simulation::HistoryView {history, time, last_value, max_history};
    auto from = view.from(time_t {90us});
    ASSERT_THAT(view.end() - from, 2);

    auto value = *from;
    ASSERT_THAT(value.first_time, time_t::min());
    ASSERT_THAT(value.last_time, time_t {95us} - min_rep);
    ASSERT_THAT(value.value, true);
}

TEST(SimulationTest, HistoryViewFromSmall) {
    auto time = time_t {100us};
    auto max_history = history_t {10us};
    auto history = Simulation::history_vector_t {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = Simulation::HistoryView {history, time, last_value, max_history};
    auto from = view.from(time_t {50us});
    ASSERT_THAT(view.end() - from, 2);
}

// until

TEST(SimulationTest, HistoryViewUntil) {
    auto time = time_t {100us};
    auto max_history = history_t {10us};
    auto history = Simulation::history_vector_t {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = Simulation::HistoryView {history, time, last_value, max_history};

    auto from = view.from(time_t {90us});
    auto until = view.until(time_t {96us});
    ASSERT_THAT(view.end() - from, 2);
    ASSERT_THAT(until - from, 2);
}

TEST(SimulationTest, HistoryViewUntilExact) {
    auto time = time_t {100us};
    auto max_history = history_t {10us};
    auto history = Simulation::history_vector_t {time_t {90us}, time_t {95us}};
    auto last_value = false;
    constexpr auto min_rep = ++time_t::zero();

    auto view = Simulation::HistoryView {history, time, last_value, max_history};

    auto from = view.from(time_t {90us});
    ASSERT_THAT(view.end() - from, 2);

    ASSERT_THAT(view.until(time_t {95us}) - from, 2);
    ASSERT_THAT(view.until(time_t {95us} - min_rep) - from, 1);
}

TEST(SimulationTest, HistoryViewFromUntilBounds) {
    auto time = time_t {100us};
    auto max_history = history_t {10us};
    auto history = Simulation::history_vector_t {time_t {90us}, time_t {95us}};
    auto last_value = false;

    auto view = Simulation::HistoryView {history, time, last_value, max_history};

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
    auto max_history = history_t {50us};
    auto history = Simulation::history_vector_t {time_t {90us}, time_t {95us}};
    auto last_value = false;
    constexpr auto min_rep = ++time_t::zero();

    auto view = Simulation::HistoryView {history, time, last_value, max_history};

    ASSERT_THAT(view.value(time_t::min()), false);
    ASSERT_THAT(view.value(time_t {-100us}), false);
    ASSERT_THAT(view.value(time_t {0us}), false);

    ASSERT_THAT(view.value(time_t {90us} - min_rep), false);
    ASSERT_THAT(view.value(time_t {90us}), true);

    ASSERT_THAT(view.value(time_t {95us} - min_rep), true);
    ASSERT_THAT(view.value(time_t {95us}), false);

    ASSERT_THAT(view.value(time_t {100us}), false);
}

TEST(SimulationTest, HistoryViewValuePartialHistory) {
    auto time = time_t {100us};
    auto max_history = history_t {10us};
    auto history = Simulation::history_vector_t {time_t {90us}, time_t {95us}};
    auto last_value = false;
    constexpr auto min_rep = ++time_t::zero();

    auto view = Simulation::HistoryView {history, time, last_value, max_history};

    ASSERT_THAT(view.value(time_t::min()), true);
    ASSERT_THAT(view.value(time_t {-100us}), true);
    ASSERT_THAT(view.value(time_t {0us}), true);

    ASSERT_THAT(view.value(time_t {90us} - min_rep), true);
    ASSERT_THAT(view.value(time_t {90us}), true);

    ASSERT_THAT(view.value(time_t {95us} - min_rep), true);
    ASSERT_THAT(view.value(time_t {95us}), false);

    ASSERT_THAT(view.value(time_t {100us}), false);
}

}  // namespace logicsim
