
#include <gtest/gtest.h>

#include "simulation.h"


namespace logicsim {

	// SimulationEvent

	TEST(SimulationEventTest, EqualOperatorTest) {
		SimulationEvent event1 { 123.456, 1, 2, true };
		SimulationEvent event2 { 123.456, 1, 2, true };
		EXPECT_TRUE(event1 == event2);


		SimulationEvent event3 { 123.456, 1, 3, true };
		SimulationEvent event4 { 123.456, 1, 2, false };
		EXPECT_TRUE(event3 == event4);
	}

	TEST(SimulationEventTest, LessThanOperatorTest) {
		SimulationEvent event1 { 123.456, 1, 2, true };
		SimulationEvent event2 { 789.1011, 3, 4, false };
		EXPECT_TRUE(event1 < event2);

		SimulationEvent event3 { 123.456, 1, 4, true };
		SimulationEvent event4 { 123.456, 3, 2, false };
		EXPECT_TRUE(event3 < event4);
	}

	TEST(SimulationEventTest, NotEqualOperatorTest) {
		SimulationEvent event1 { 123.456, 1, 2, true };
		SimulationEvent event2 { 789.1011, 3, 4, false };
		EXPECT_TRUE(event1 != event2);
	}

	TEST(SimulationEventTest, GreaterThanOperatorTest) {
		SimulationEvent event1 { 123.456, 1, 2, true };
		SimulationEvent event2 { 789.1011, 3, 4, false };
		EXPECT_TRUE(event2 > event1);
	}

	TEST(SimulationEventTest, LessThanOrEqualOperatorTest) {
		SimulationEvent event1 { 123.456, 1, 2, true };
		SimulationEvent event2 { 789.1011, 3, 4, false };
		EXPECT_TRUE(event1 <= event2);
	}

	TEST(SimulationEventTest, GreaterThanOrEqualOperatorTest) {
		SimulationEvent event1 { 123.456, 1, 2, true };
		SimulationEvent event2 { 789.1011, 3, 4, false };
		EXPECT_TRUE(event2 >= event1);
	}

	// Simulation

	TEST(SimulationTest, InitializeSimulation) {
		Circuit circuit;
		circuit.add_element(ElementType::inverter_element, 1, 1);

		add_output_placeholders(circuit);
		SimulationState state { circuit.total_input_count() };
		initialize_simulation(state, circuit);

		advance_simulation(state, circuit, 0, true);
	}
}
