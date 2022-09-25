#ifndef LOGIKSIM_CIRCUIT_H
#define LOGIKSIM_CIRCUIT_H

#include <ranges>
#include <cstdint>
#include <vector>

namespace logicsim {

	enum class ElementType {
		wire,
		xor_element,
		and_element,
		or_element
	};


	using element_size_t = int32_t;
	using connection_size_t = int8_t;

	constexpr element_size_t null_element = -1;
	constexpr connection_size_t null_connection = -1;


	class CircuitGraph {
	private:
		struct Element {
			ElementType type;
			int input_index;
			int output_index;
			connection_size_t input_count;
			connection_size_t output_count;
		};
		struct Connectivity {
			element_size_t element = null_element;
			connection_size_t index = null_connection;
		};

		std::vector<Element> elements_;
		std::vector<Connectivity> outputs_;
		std::vector<Connectivity> inputs_;

		Element& get_element_node(element_size_t element) {
			return elements_.at(element);
		}

		const Element& get_element_node(element_size_t element) const
		{
			return elements_.at(element);
		}

		Connectivity& get_output_node(const Element& element_node, connection_size_t output) {
			if (output < 0 || output >= element_node.output_count) {
				throw std::exception("Output index is invalid.");
			}
			return outputs_.at(element_node.output_index + output);
		}

		Connectivity& get_input_node(const Element& element_node, connection_size_t input) {
			if (input < 0 || input >= element_node.input_count) {
				throw std::exception("Output index is invalid.");
			}
			return inputs_.at(element_node.input_index + input);
		}

	public:
		element_size_t create_element(
			ElementType type,
			connection_size_t input_count,
			connection_size_t output_count)
		{
			element_size_t element = static_cast<element_size_t>(elements_.size());

			elements_.push_back({ type, static_cast<int>(inputs_.size()), static_cast<int>(outputs_.size()), input_count, output_count });
			inputs_.resize(inputs_.size() + input_count);
			outputs_.resize(outputs_.size() + output_count);

			return element;
		}

		element_size_t element_count()
		{
			return static_cast<element_size_t>(elements_.size());
		}

		void connect_output(
			element_size_t from_element,
			connection_size_t from_output,
			element_size_t to_element = null_element,
			connection_size_t to_input = null_connection)
		{
			Connectivity& from_conn = get_output_node(get_element_node(from_element), from_output);
			Connectivity& to_conn = get_input_node(get_element_node(to_element), to_input);

			from_conn.element = to_element;
			from_conn.index = to_input;

			to_conn.element = from_element;
			to_conn.index = from_output;
		}

		ElementType get_type(
			element_size_t element) const
		{
			return get_element_node(element).type;
		}

		element_size_t get_connected_element(
			element_size_t element,
			connection_size_t output)
		{
			return get_output_node(get_element_node(element), output).element;
		}

		connection_size_t get_connected_input(
			element_size_t element,
			connection_size_t output)
		{
			return get_output_node(get_element_node(element), output).index;
		}

		connection_size_t get_input_count(
			[[maybe_unused]] element_size_t element)
		{
			return get_element_node(element).input_count;
		}

		connection_size_t get_output_count(
			element_size_t element)
		{
			return get_element_node(element).output_count;
		}
	};


	template <class T>
	int benchmark_graph(const int n_elements = 100) {

		T graph{};

		auto elem0 = graph.create_element(ElementType::and_element, 2, 2);

		for ([[maybe_unused]] auto _ : std::ranges::iota_view(1, n_elements)) {
			const auto wire0 = graph.create_element(ElementType::wire, 1, 1);
			const auto wire1 = graph.create_element(ElementType::wire, 1, 1);
			const auto elem1 = graph.create_element(ElementType::and_element, 2, 2);

			graph.connect_output(elem0, 0, wire0, 0);
			graph.connect_output(elem0, 1, wire1, 0);

			graph.connect_output(wire0, 0, elem1, 0);
			graph.connect_output(wire1, 0, elem1, 1);

			elem0 = elem1;
		}

		return 1;
	}
}

#endif