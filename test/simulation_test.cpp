
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
    simulation.advance();

    EXPECT_EQ(simulation.input_value(inverter.input(0)), false);
    EXPECT_EQ(simulation.output_value(inverter.output(0)), true);
}

TEST(SimulationTest, SimulationTimeAdvancingWithoutEvents) {
    using namespace std::chrono_literals;
    Circuit circuit;

    auto simulation = get_initialized_simulation(circuit);

    EXPECT_EQ(simulation.time(), 0us);
    simulation.advance(3s);
    EXPECT_EQ(simulation.time(), 3s);
}

TEST(SimulationTest, SimulationTimeAdvancingWithoutInfiniteEvents) {
    using namespace std::chrono_literals;

    // create infinite loop
    Circuit circuit;
    const auto inverter = circuit.add_element(ElementType::inverter_element, 1, 1);
    inverter.output(0).connect(inverter.input(0));

    auto simulation = get_uninitialized_simulation(circuit);
    simulation.set_output_delay(inverter.output(0), 100us);
    simulation.initialize();

    EXPECT_EQ(simulation.time(), 0us);
    simulation.advance(5ms);
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
    simulation.advance(Simulation::defaults::infinite_simulation_time, 5ms);
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
    simulation.advance();

    EXPECT_EQ(simulation.input_value(xor_element.input(0)), false);
    EXPECT_EQ(simulation.input_value(xor_element.input(1)), false);
    EXPECT_EQ(simulation.output_value(xor_element.output(0)), false);

    // enable first input
    simulation.submit_event(xor_element.input(0), 10us, true);
    simulation.advance();

    EXPECT_EQ(simulation.input_value(xor_element.input(0)), true);
    EXPECT_EQ(simulation.input_value(xor_element.input(1)), false);
    EXPECT_EQ(simulation.output_value(xor_element.output(0)), true);

    // enable second input
    simulation.submit_event(xor_element.input(1), 10us, true);
    simulation.advance();

    EXPECT_EQ(simulation.input_value(xor_element.input(0)), true);
    EXPECT_EQ(simulation.input_value(xor_element.input(1)), true);
    EXPECT_EQ(simulation.output_value(xor_element.output(0)), false);
}

TEST(SimulationTest, SimulatanousEvents) {
    Circuit circuit;
    auto xor_element {circuit.add_element(ElementType::xor_element, 2, 1)};

    auto simulation = get_initialized_simulation(circuit);
    simulation.submit_event(xor_element.input(0), 10us, true);
    simulation.advance();

    EXPECT_EQ(simulation.input_value(xor_element.input(0)), true);
    EXPECT_EQ(simulation.input_value(xor_element.input(1)), false);
    EXPECT_EQ(simulation.output_value(xor_element.output(0)), true);

    // flip inputs at the same time
    simulation.submit_event(xor_element.input(0), 10us, false);
    simulation.submit_event(xor_element.input(1), 10us, true);
    simulation.advance();

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
        simulation.advance();

        EXPECT_EQ(simulation.output_value(output.output(0)), false);
        EXPECT_EQ(simulation.output_value(carry.output(0)), false);
    }

    // 0 + 1 = 01
    {
        simulation.submit_event(input0.input(0), 10us, true);
        simulation.submit_event(input1.input(0), 10us, false);
        simulation.advance();

        EXPECT_EQ(simulation.output_value(output.output(0)), true);
        EXPECT_EQ(simulation.output_value(carry.output(0)), false);
    }

    // 1 + 0 = 01
    {
        simulation.submit_event(input0.input(0), 10us, false);
        simulation.submit_event(input1.input(0), 10us, true);
        simulation.advance();

        EXPECT_EQ(simulation.output_value(output.output(0)), true);
        EXPECT_EQ(simulation.output_value(carry.output(0)), false);
    }

    // 1 + 1 = 10
    {
        simulation.submit_event(input0.input(0), 10us, true);
        simulation.submit_event(input1.input(0), 10us, true);
        simulation.advance();

        EXPECT_EQ(simulation.output_value(output.output(0)), false);
        EXPECT_EQ(simulation.output_value(carry.output(0)), true);
    }
}

TEST(SimulationTest, OutputDelayTest) {
    using namespace std::chrono_literals;

    Circuit circuit;
    const auto wire {circuit.add_element(ElementType::wire, 1, 3)};
    auto simulation = get_initialized_simulation(circuit);

    simulation.set_output_delay(wire.output(0), 1s);
    simulation.set_output_delay(wire.output(1), 2s);
    simulation.set_output_delay(wire.output(2), 3s);

    simulation.submit_event(wire.input(0), 1us, true);
    simulation.advance(1us);

    // after 0.5 seconds
    simulation.advance(500ms);
    ASSERT_THAT(simulation.output_values(wire), testing::ElementsAre(0, 0, 0));
    // after 1.5 seconds
    simulation.advance(1s);
    ASSERT_THAT(simulation.output_values(wire), testing::ElementsAre(1, 0, 0));
    // after 2.5 seconds
    simulation.advance(1s);
    ASSERT_THAT(simulation.output_values(wire), testing::ElementsAre(1, 1, 0));
    // after 3.5 seconds
    simulation.advance(1s);
    ASSERT_THAT(simulation.output_values(wire), testing::ElementsAre(1, 1, 1));
}

}  // namespace logicsim
