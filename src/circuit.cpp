
#include "circuit.h"

#include <ranges>
#include <algorithm>


namespace logicsim {

	void validate_output_connected(Circuit::OutputConnection output) {
		if (!output.has_connected_element()) {
			throw_exception("Element has unconnected output.");
		}
	}

	void validate_outputs_connected(Circuit::Element element) {
		std::ranges::for_each(element.outputs(), validate_output_connected);
	}

	void validate_input_consistent(Circuit::InputConnection input) {
		if (input.has_connected_element()) {
			auto back_reference = input.connected_output().connected_input();
			if (back_reference != input) {
				throw_exception("Back reference doesn't match.");
			}
		}
	}

	void validate_output_consistent(Circuit::OutputConnection output) {
		if (output.has_connected_element()) {
			auto back_reference = output.connected_input().connected_output();
			if (back_reference != output) {
				throw_exception("Back reference doesn't match.");
			}
		}
	}

	void validate_element_connections_consistent(Circuit::Element element) {
		std::ranges::for_each(element.inputs(), validate_input_consistent);
		std::ranges::for_each(element.outputs(), validate_output_consistent);
	}

	void Circuit::validate_connection_data_(Circuit::ConnectionData connection_data) {
		if (connection_data.element_id != null_element &&
			connection_data.index == null_connection)
		{
			throw_exception("Connection to an element cannot have null_connection.");
		}

		if (connection_data.element_id == null_element &&
			connection_data.index != null_connection)
		{
			throw_exception("Connection with null_element requires null_connection.");
		}
	}

	void Circuit::validate(bool require_all_outputs_connected) {
		auto all_one = [](auto vector) {
			return std::ranges::all_of(vector, [](auto item) {return item == 1; });
		};

		//  every output_data entry is referenced once
		std::vector<int> input_reference_count(total_input_count(), 0);
		for (auto element : elements()) {
			for (auto input : element.inputs()) {
				input_reference_count.at(input.input_id()) += 1;
			}
		}
		if (!all_one(input_reference_count)) {
			throw_exception("Input data is inconsistent");
		}

		//  every output_data entry is referenced once
		std::vector<int> output_reference_count(total_output_count(), 0);
		for (auto element : elements()) {
			for (auto output : element.outputs()) {
				output_reference_count.at(output.output_id()) += 1;
			}
		}
		if (!all_one(output_reference_count)) {
			throw_exception("Input data is inconsistent");
		}

		// connection data valid
		std::ranges::for_each(input_data_store_, Circuit::validate_connection_data_);
		std::ranges::for_each(output_data_store_, Circuit::validate_connection_data_);

		// back references consistent
		std::ranges::for_each(elements(), validate_element_connections_consistent);

		// all outputs connected
		if (require_all_outputs_connected) {
			std::ranges::for_each(elements(), validate_outputs_connected);
		}
	}


	void create_placeholder(Circuit::OutputConnection output) {
		if (!output.has_connected_element()) {
			auto placeholder = output.circuit()->create_element(
				ElementType::input_placeholder, 1, 0);
			output.connect(placeholder.input(0));
		}
	}

	void create_element_placeholders(Circuit::Element element) {
		std::ranges::for_each(element.outputs(), create_placeholder);
	}

	void create_placeholders(Circuit &circuit) {
		std::ranges::for_each(circuit.elements(), create_element_placeholders);
	}
	



	Circuit benchmark_circuit(const int n_elements) {

		Circuit circuit{};

		auto elem0 = circuit.create_element(ElementType::and_element, 2, 2);

		for ([[maybe_unused]] auto _ : std::ranges::iota_view(1, n_elements)) {
			auto wire0 = circuit.create_element(ElementType::wire, 1, 1);
			auto wire1 = circuit.create_element(ElementType::wire, 1, 1);
			auto elem1 = circuit.create_element(ElementType::and_element, 2, 2);

			elem0.output(0).connect(wire0.input(0));
			elem0.output(1).connect(wire1.input(0));

			wire0.output(0).connect(elem1.input(0));
			wire1.output(0).connect(elem1.input(1));

			elem0 = elem1;
		}

		return circuit;
	}

}
