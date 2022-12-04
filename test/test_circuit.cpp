
#include "circuit.h"

#include <gtest/gtest.h>

#include <range/v3/all.hpp>


namespace logicsim {

	TEST(Circuit, EmptyCircuit) {
		const Circuit circuit;

		EXPECT_EQ(circuit.element_count(), 0);
		EXPECT_EQ(circuit.total_input_count(), 0);
		EXPECT_EQ(circuit.total_output_count(), 0);
		EXPECT_EQ(ranges::distance(circuit.elements()), 0);

		circuit.validate();
	}


	TEST(Circuit, CircuitSingleElement) {
		Circuit circuit;

		circuit.create_element(ElementType::wire, 3, 5);

		EXPECT_EQ(circuit.element_count(), 1);
		EXPECT_EQ(circuit.total_input_count(), 3);
		EXPECT_EQ(circuit.total_output_count(), 5);
		EXPECT_EQ(ranges::distance(circuit.elements()), 1);

		circuit.validate();
	}

	TEST(Circuit, ElementProperties) {
		Circuit circuit;
		circuit.create_element(ElementType::wire, 3, 5);

		const Circuit& circuit_const { circuit };
		const Circuit::ConstElement element { circuit_const.element(0) };

		EXPECT_EQ(element.element_id(), 0);
		EXPECT_EQ(element.element_type(), ElementType::wire);
		EXPECT_EQ(element.input_count(), 3);
		EXPECT_EQ(element.output_count(), 5);

		EXPECT_EQ(ranges::distance(element.inputs()), 3);
		EXPECT_EQ(ranges::distance(element.outputs()), 5);

		circuit.validate();
		circuit_const.validate();
	}


	TEST(Circuit, EqualityOperators) {
		Circuit circuit;

		auto wire { circuit.create_element(ElementType::wire, 3, 5) };
		auto inverter { circuit.create_element(ElementType::inverter_element, 3, 2) };

		const Circuit& circuit_const { circuit };

		EXPECT_EQ(wire, wire);
		EXPECT_EQ(wire, circuit_const.element(0));
		EXPECT_NE(wire, inverter);

		EXPECT_EQ(wire.output(0), wire.output(0));
		EXPECT_EQ(wire.output(0), circuit_const.element(0).output(0));
		EXPECT_NE(wire.output(0), inverter.output(0));
		EXPECT_NE(wire.output(0), wire.output(1));
		EXPECT_NE(wire.output(0), circuit_const.element(0).output(1));

		circuit_const.validate();
		circuit.validate();
	}

	TEST(Circuit, ConnectionProperties) {
		Circuit circuit;

		auto wire { circuit.create_element(ElementType::wire, 3, 5) };
		auto inverter { circuit.create_element(ElementType::inverter_element, 3, 2) };

		EXPECT_EQ(wire.output(1).element_id(), wire.element_id());
		EXPECT_EQ(wire.output(1).output_index(), 1);
		EXPECT_EQ(wire.output(1).element(), wire);
		EXPECT_EQ(wire.output(1).has_connected_element(), false);

		EXPECT_EQ(inverter.input(1).element_id(), inverter.element_id());
		EXPECT_EQ(inverter.input(1).input_index(), 1);
		EXPECT_EQ(inverter.input(1).element(), inverter);
		EXPECT_EQ(inverter.input(1).has_connected_element(), false);

		circuit.validate();
	}

	TEST(Circuit, ConnectedOutput) {
		Circuit circuit;

		auto wire { circuit.create_element(ElementType::wire, 3, 5) };
		auto inverter { circuit.create_element(ElementType::inverter_element, 3, 2) };

		wire.output(1).connect(inverter.input(1));

		EXPECT_EQ(wire.output(1).has_connected_element(), true);
		EXPECT_EQ(wire.output(1).connected_element_id(), inverter.element_id());
		EXPECT_EQ(wire.output(1).connected_element(), inverter);
		EXPECT_EQ(wire.output(1).connected_input(), inverter.input(1));

		EXPECT_EQ(inverter.input(1).has_connected_element(), true);
		EXPECT_EQ(inverter.input(1).connected_element_id(), wire.element_id());
		EXPECT_EQ(inverter.input(1).connected_element(), wire);
		EXPECT_EQ(inverter.input(1).connected_output(), wire.output(1));

		circuit.validate();
	}

	TEST(Circuit, ConnectInput) {
		Circuit circuit;

		auto wire { circuit.create_element(ElementType::wire, 3, 5) };
		auto inverter { circuit.create_element(ElementType::inverter_element, 3, 2) };

		inverter.input(1).connect(wire.output(1));

		EXPECT_EQ(wire.output(1).has_connected_element(), true);
		EXPECT_EQ(wire.output(1).connected_element_id(), inverter.element_id());
		EXPECT_EQ(wire.output(1).connected_element(), inverter);
		EXPECT_EQ(wire.output(1).connected_input(), inverter.input(1));

		EXPECT_EQ(inverter.input(1).has_connected_element(), true);
		EXPECT_EQ(inverter.input(1).connected_element_id(), wire.element_id());
		EXPECT_EQ(inverter.input(1).connected_element(), wire);
		EXPECT_EQ(inverter.input(1).connected_output(), wire.output(1));

		circuit.validate();
	}

	TEST(Circuit, ClearedInput) {
		Circuit circuit;

		auto wire { circuit.create_element(ElementType::wire, 3, 5) };
		auto inverter { circuit.create_element(ElementType::inverter_element, 3, 2) };

		wire.output(1).connect(inverter.input(1));
		inverter.input(1).clear_connection();

		EXPECT_EQ(inverter.input(1).has_connected_element(), false);
		EXPECT_EQ(wire.output(1).has_connected_element(), false);

		circuit.validate();
	}

	TEST(Circuit, ClearedOutput) {
		Circuit circuit;

		auto wire { circuit.create_element(ElementType::wire, 3, 5) };
		auto inverter { circuit.create_element(ElementType::inverter_element, 3, 2) };

		wire.output(1).connect(inverter.input(1));
		wire.output(1).clear_connection();

		EXPECT_EQ(inverter.input(1).has_connected_element(), false);
		EXPECT_EQ(wire.output(1).has_connected_element(), false);

		circuit.validate();
	}

	TEST(Circuit, ReconnectInput) {
		Circuit circuit;

		auto wire { circuit.create_element(ElementType::wire, 3, 5) };
		auto inverter { circuit.create_element(ElementType::inverter_element, 3, 2) };

		wire.output(1).connect(inverter.input(1));
		inverter.input(1).connect(inverter.output(1));

		EXPECT_EQ(wire.output(1).has_connected_element(), false);
		EXPECT_EQ(inverter.input(1).has_connected_element(), true);
		EXPECT_EQ(inverter.output(1).has_connected_element(), true);

		circuit.validate();
	}

	TEST(Circuit, ReconnectOutput) {
		Circuit circuit;

		auto wire { circuit.create_element(ElementType::wire, 3, 5) };
		auto inverter { circuit.create_element(ElementType::inverter_element, 3, 2) };

		wire.output(1).connect(inverter.input(1));
		wire.output(1).connect(wire.input(1));

		EXPECT_EQ(wire.output(1).has_connected_element(), true);
		EXPECT_EQ(inverter.input(1).has_connected_element(), false);
		EXPECT_EQ(wire.input(1).has_connected_element(), true);

		circuit.validate();
	}

	TEST(Circuit, TestPlaceholders) {
		Circuit circuit;
		auto wire { circuit.create_element(ElementType::wire, 3, 5) };
		EXPECT_EQ(circuit.element_count(), 1);

		create_output_placeholders(circuit);
		EXPECT_EQ(circuit.element_count(), 6);

		EXPECT_EQ(wire.output(3).has_connected_element(), true);
		EXPECT_EQ(wire.output(3).connected_element().element_type(), ElementType::placeholder);

		circuit.validate();
		circuit.validate(true);
	}


}
