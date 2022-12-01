#ifndef LOGIKSIM_CIRCUIT_H
#define LOGIKSIM_CIRCUIT_H

#include <ranges>
#include <cstdint>
#include <vector>
#include <iostream>

#include "algorithms.h"


namespace logicsim {

	enum class ElementType : uint8_t {
		input_placeholder,
		wire,
		inverter_element,
		and_element,
		or_element,
		xor_element
	};


	using element_id_t = int32_t;
	using connection_index_t = int32_t;
	using connection_size_t = int8_t;

	constexpr element_id_t null_element = -1;
	constexpr connection_size_t null_connection = -1;


	template <typename Output, typename Vector>
	Output& get_connectivity(Vector& vector, connection_index_t vec_index, connection_size_t count, connection_size_t con_index) {
		if (con_index < 0 || con_index >= count) {
			throw_exception("Index is invalid");
		}
		return vector.at(vec_index + con_index);
	}


	struct ElementInputConfig {
		connection_index_t input_index;
		connection_size_t input_count;
		ElementType type;
	};

	struct Connectivity {
		element_id_t element = null_element;
		connection_size_t index = null_connection;
	};


	class CircuitElement;

	class CircuitGraph {
	friend CircuitElement;
	
	private:

		struct Element {
			connection_index_t input_index;
			connection_index_t output_index;

			connection_size_t input_count;
			connection_size_t output_count;

			ElementType type;
		};

		std::vector<Element> elements_;
		std::vector<Connectivity> outputs_;
		std::vector<Connectivity> inputs_;

		Element& get_element_node(element_id_t element) {
			return elements_.at(element);
		}

		const Element& get_element_node(element_id_t element) const
		{
			return elements_.at(element);
		}

		Connectivity& get_output_con(const Element& element_node, connection_size_t output) {
			return get_connectivity<Connectivity>(outputs_, element_node.output_index, element_node.output_count, output);
		}

		const Connectivity& get_output_con(const Element& element_node, connection_size_t output) const {
			return get_connectivity<const Connectivity>(outputs_, element_node.output_index, element_node.output_count, output);
		}

		Connectivity& get_input_con(const Element& element_node, connection_size_t input) {
			return get_connectivity<Connectivity>(inputs_, element_node.input_index, element_node.input_count, input);
		}

		const Connectivity& get_input_con(const Element& element_node, connection_size_t input) const {
			return get_connectivity<const Connectivity>(inputs_, element_node.input_index, element_node.input_count, input);
		}

	public:
		element_id_t create_element(
			ElementType type,
			connection_size_t input_count,
			connection_size_t output_count)
		{
			element_id_t element = static_cast<element_id_t>(elements_.size());

			elements_.push_back({ static_cast<int>(inputs_.size()), static_cast<int>(outputs_.size()), input_count, output_count, type });
			inputs_.resize(inputs_.size() + input_count);
			outputs_.resize(outputs_.size() + output_count);

			return element;
		}

		auto elements() const {
			return std::views::iota(0, element_count());
		}

		auto inputs(element_id_t element) const {
			return std::views::iota(0, get_input_count(element));
		}

		auto outputs(element_id_t element) const {
			return std::views::iota(0, get_output_count(element));
		}

		element_id_t element_count() const noexcept
		{
			return static_cast<element_id_t>(elements_.size());
		}

		auto total_inputs() const noexcept {
			return inputs_.size();
		}

		auto total_outputs() const noexcept {
			return outputs_.size();
		}

		void connect_output(
			element_id_t from_element,
			connection_size_t from_output,
			element_id_t to_element = null_element,
			connection_size_t to_input = null_connection)
		{
			Connectivity& from_con = get_output_con(get_element_node(from_element), from_output);
			Connectivity& to_con = get_input_con(get_element_node(to_element), to_input);

			from_con.element = to_element;
			from_con.index = to_input;

			to_con.element = from_element;
			to_con.index = from_output;
		}

		ElementType get_type(
			element_id_t element) const
		{
			return get_element_node(element).type;
		}

		element_id_t get_connected_element(
			element_id_t element,
			connection_size_t output) const
		{
			return get_output_con(get_element_node(element), output).element;
		}

		connection_size_t get_connected_input(
			element_id_t element,
			connection_size_t output) const
		{
			return get_output_con(get_element_node(element), output).index;
		}

		connection_size_t get_input_count(element_id_t element) const
		{
			return get_element_node(element).input_count;
		}

		connection_size_t get_output_count(element_id_t element) const
		{
			return get_element_node(element).output_count;
		}

		connection_index_t get_input_index(element_id_t element) const {
			return get_element_node(element).input_index;
		}

		connection_index_t get_input_index(element_id_t element, connection_size_t input) const {
			return get_input_index(element) + input;
		}

		connection_index_t get_output_index(element_id_t element) const {
			return get_element_node(element).output_index;
		}

		connection_index_t get_output_index(element_id_t element, connection_size_t output) const {
			return get_output_index(element) + output;
		}

		ElementInputConfig get_input_config(element_id_t element) const
		{
			const auto node = get_element_node(element);
			return ElementInputConfig{ node.input_index, node.input_count, node.type};
		}


		Connectivity get_output_connectivty(element_id_t element, connection_size_t output) const {
			return get_output_con(get_element_node(element), output);
		}

		Connectivity get_input_connectivity(element_id_t element, connection_size_t input) const {
			return get_input_con(get_element_node(element), input);
		}
	};

	// CircuitGraph create_placeholders(CircuitGraph graph);

	CircuitGraph& create_placeholders(CircuitGraph& graph);


	class CircuitElement {
	public:
		CircuitElement(CircuitGraph& graph, element_id_t element_id) :
			graph(graph), 
			element_id(element_id)
		{
		}

		element_id_t get_element_id() {
			return element_id;
		}

		ElementType get_type() const
		{
			return graph.get_element_node(element_id).type;
		}

		connection_size_t get_input_count() const
		{
			return graph.get_element_node(element_id).input_count;
		}

		connection_size_t get_output_count() const
		{
			return graph.get_element_node(element_id).output_count;
		}

		connection_index_t get_input_index() const {
			return graph.get_element_node(element_id).input_index;
		}

		connection_index_t get_input_index(connection_size_t input) const {
			if (input < 0 || input >= get_input_count()) {
				throw_exception("Index is invalid");
			}
			return get_input_index() + input;
		}

		connection_index_t get_output_index() const {
			return graph.get_element_node(element_id).output_index;
		}

		connection_index_t get_output_index(connection_size_t output) const {
			if (output < 0 || output >= get_output_count()) {
				throw_exception("Index is invalid");
			}
			return get_output_index() + output;
		}


	private:
		CircuitGraph& graph;
		element_id_t element_id;
	};

	class InputConnectivity {
	public:
		InputConnectivity(CircuitElement& element, CircuitGraph &graph) : 
			graph(graph),
			element(element)
		{
		}

	private:
		CircuitGraph& graph;
		CircuitElement &element;
	};



	template <class T>
	T benchmark_graph(const int n_elements = 100) {

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

		return graph;
	}
}

#endif