
#include "simulation.h"

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

TEST(SimulationTest, InitializeSimulation) {
    Circuit circuit;
    auto inverter {circuit.add_element(ElementType::inverter_element, 1, 1)};

    auto state {simulate_circuit(circuit)};

    EXPECT_EQ(get_input_value(inverter.input(0), state), false);
    EXPECT_EQ(get_output_value(inverter.output(0), state), true);
}

TEST(SimulationTest, SimulationTimeAdvancingWithoutEvents) {
    using namespace std::chrono_literals;
    Circuit circuit;

    auto state {get_initialized_state(circuit)};

    EXPECT_EQ(state.queue.time(), 0us);
    advance_simulation(state, circuit, 3s);
    EXPECT_EQ(state.queue.time(), 3s);
}

TEST(SimulationTest, SimulationTimeAdvancingWithoutInfiniteEvents) {
    using namespace std::chrono_literals;

    // create infinite loop
    Circuit circuit;
    const auto inverter = circuit.add_element(ElementType::inverter_element, 1, 1);
    inverter.output(0).connect(inverter.input(0));

    auto state {get_uninitialized_state(circuit)};
    set_output_delay(inverter.output(0), state, 100us);
    initialize_simulation(state, circuit);

    EXPECT_EQ(state.queue.time(), 0us);
    advance_simulation(state, circuit, 5ms);
    EXPECT_EQ(state.queue.time(), 5ms);
}

TEST(SimulationTest, SimulationInfiniteEventsTimeout) {
    using namespace std::chrono_literals;

    // create infinite loop
    Circuit circuit;
    const auto inverter = circuit.add_element(ElementType::inverter_element, 1, 1);
    inverter.output(0).connect(inverter.input(0));
    auto state {get_initialized_state(circuit)};

    // run simulation for 5 ms
    EXPECT_EQ(state.queue.time(), 0us);
    const auto start = timeout_clock::now();
    advance_simulation(state, circuit, defaults::infinite_simulation_time, 5ms);
    const auto end = timeout_clock::now();

    EXPECT_GT(state.queue.time(), 1ms);
    const auto delay = end - start;
    ASSERT_THAT(delay > 5ms, true);
    ASSERT_THAT(delay < 6ms, true);
}

TEST(SimulationTest, AdditionalEvents) {
    Circuit circuit;
    auto xor_element {circuit.add_element(ElementType::xor_element, 2, 1)};

    auto state {get_initialized_state(circuit)};
    advance_simulation(state, circuit);

    EXPECT_EQ(get_input_value(xor_element.input(0), state), false);
    EXPECT_EQ(get_input_value(xor_element.input(1), state), false);
    EXPECT_EQ(get_output_value(xor_element.output(0), state), false);

    // enable first input
    const auto t1 {state.queue.time() + 10us};
    state.queue.submit_event(make_event(xor_element.input(0), t1, true));
    advance_simulation(state, circuit);

    EXPECT_EQ(get_input_value(xor_element.input(0), state), true);
    EXPECT_EQ(get_input_value(xor_element.input(1), state), false);
    EXPECT_EQ(get_output_value(xor_element.output(0), state), true);

    // enable second input
    const auto t2 {state.queue.time() + 10us};
    state.queue.submit_event(make_event(xor_element.input(1), t2, true));
    advance_simulation(state, circuit);

    EXPECT_EQ(get_input_value(xor_element.input(0), state), true);
    EXPECT_EQ(get_input_value(xor_element.input(1), state), true);
    EXPECT_EQ(get_output_value(xor_element.output(0), state), false);
}

TEST(SimulationTest, SimulatanousEvents) {
    Circuit circuit;
    auto xor_element {circuit.add_element(ElementType::xor_element, 2, 1)};

    auto state {get_initialized_state(circuit)};
    state.queue.submit_event(make_event(xor_element.input(0), 10us, true));
    advance_simulation(state, circuit);

    EXPECT_EQ(get_input_value(xor_element.input(0), state), true);
    EXPECT_EQ(get_input_value(xor_element.input(1), state), false);
    EXPECT_EQ(get_output_value(xor_element.output(0), state), true);

    // flip inputs at the same time
    const auto t1 {state.queue.time() + 10us};
    state.queue.submit_event(make_event(xor_element.input(0), t1, false));
    state.queue.submit_event(make_event(xor_element.input(1), t1, true));
    advance_simulation(state, circuit);

    EXPECT_EQ(get_input_value(xor_element.input(0), state), false);
    EXPECT_EQ(get_input_value(xor_element.input(1), state), true);
    EXPECT_EQ(get_output_value(xor_element.output(0), state), true);
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

    auto state = get_initialized_state(circuit);

    // 0 + 0 -> 00
    {
        const auto t {state.queue.time() + 10us};
        state.queue.submit_event(make_event(input0.input(0), t, false));
        state.queue.submit_event(make_event(input1.input(0), t, false));
        advance_simulation(state, circuit);

        EXPECT_EQ(get_output_value(output.output(0), state), false);
        EXPECT_EQ(get_output_value(carry.output(0), state), false);
    }

    // 0 + 1 = 01
    {
        const auto t {state.queue.time() + 10us};
        state.queue.submit_event(make_event(input0.input(0), t, true));
        state.queue.submit_event(make_event(input1.input(0), t, false));
        advance_simulation(state, circuit);

        EXPECT_EQ(get_output_value(output.output(0), state), true);
        EXPECT_EQ(get_output_value(carry.output(0), state), false);
    }

    // 1 + 0 = 01
    {
        const auto t {state.queue.time() + 10us};
        state.queue.submit_event(make_event(input0.input(0), t, false));
        state.queue.submit_event(make_event(input1.input(0), t, true));
        advance_simulation(state, circuit);

        EXPECT_EQ(get_output_value(output.output(0), state), true);
        EXPECT_EQ(get_output_value(carry.output(0), state), false);
    }

    // 1 + 1 = 10
    {
        const auto t {state.queue.time() + 10us};
        state.queue.submit_event(make_event(input0.input(0), t, true));
        state.queue.submit_event(make_event(input1.input(0), t, true));
        advance_simulation(state, circuit);

        EXPECT_EQ(get_output_value(output.output(0), state), false);
        EXPECT_EQ(get_output_value(carry.output(0), state), true);
    }
}

TEST(SimulationTest, OutputDelayTest) {
    using namespace std::chrono_literals;

    Circuit circuit;
    const auto wire {circuit.add_element(ElementType::wire, 1, 3)};
    auto state = get_initialized_state(circuit);

    set_output_delay(wire.output(0), state, 1s);
    set_output_delay(wire.output(1), state, 2s);
    set_output_delay(wire.output(2), state, 3s);

    state.queue.submit_event(make_event(wire.input(0), 1us, true));
    advance_simulation(state, circuit, 1us);

    // after 0.5 seconds
    advance_simulation(state, circuit, 500ms);
    ASSERT_THAT(get_output_values(wire, state), testing::ElementsAre(0, 0, 0));
    // after 1.5 seconds
    advance_simulation(state, circuit, 1s);
    ASSERT_THAT(get_output_values(wire, state), testing::ElementsAre(1, 0, 0));
    // after 2.5 seconds
    advance_simulation(state, circuit, 1s);
    ASSERT_THAT(get_output_values(wire, state), testing::ElementsAre(1, 1, 0));
    // after 3.5 seconds
    advance_simulation(state, circuit, 1s);
    ASSERT_THAT(get_output_values(wire, state), testing::ElementsAre(1, 1, 1));
}

}  // namespace logicsim
