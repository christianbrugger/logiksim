
#include "circuit.h"

#include <ranges>
#include <algorithm>


namespace logicsim {

	void create_placeholder(CircuitGraph& graph, element_size_t element, connection_size_t output) {
		if (graph.get_connected_element(element, output) == null_element) {
			auto placeholder = graph.create_element(ElementType::input_placeholder, 1, 0);
			graph.connect_output(element, output, placeholder, 0);
		}
	}

	void create_placeholders(CircuitGraph& graph, element_size_t element) {
		std::ranges::for_each(std::views::iota(0, graph.get_output_count(element)),
			[&graph, element](connection_size_t output) { create_placeholder(graph, element, output); });
	}

	CircuitGraph create_placeholders(CircuitGraph graph) {
		std::ranges::for_each(graph.elements(),
			[&graph](element_size_t element) { create_placeholders(graph, element); });
		return graph;
	}

}
