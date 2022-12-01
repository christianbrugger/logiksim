
#include "circuit2.h"

#include <ranges>
#include <algorithm>


namespace logicsim2 {

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
