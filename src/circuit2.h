#ifndef LOGIKSIM_CIRCUIT_H
#define LOGIKSIM_CIRCUIT_H

#include <ranges>
#include <cstdint>
#include <vector>
#include <iostream>

#include "algorithms.h"


namespace logicsim2 {

	enum class ElementType : uint8_t {
		input_placeholder,
		wire,
		inverter_element,
		and_element,
		or_element,
		xor_element
	};


	using element_id_t = int32_t;
	using connection_id_t = int32_t;
	using connection_size_t = int8_t;

	constexpr element_id_t null_element = -1;
	constexpr connection_size_t null_connection = -1;


	class CircuitGraph {

	private:
		struct ConnectivityData {
			element_id_t element_id = null_element;
			connection_size_t index = null_connection;
		};

		struct ElementData {
			connection_id_t first_input_id;
			connection_id_t first_output_id;

			connection_size_t input_count;
			connection_size_t output_count;

			ElementType type;
		};

		std::vector<ElementData> elements_;
		std::vector<ConnectivityData> outputs_;
		std::vector<ConnectivityData> inputs_;

	public:
		class Element;


		class InputConnectivity {
		private:
			friend Element;
			InputConnectivity(CircuitGraph& graph, element_id_t element_id, connection_id_t input_id) :
				graph_(graph),
				element_id_(element_id),
				input_id_(input_id)
			{
			}
		public:
			element_id_t element_id() const {
				return graph_.inputs_.at(input_id_).element_id;
			}

			bool has_element() const {
				return element_id() != null_element;
			}

			Element element() {
				return Element{ graph_, element_id() };
			}

			connection_size_t output_index() const {
				return graph_.inputs_.at(input_id_).index;
			}

		private:
			CircuitGraph& graph_;
			element_id_t element_id_;
			connection_id_t input_id_;
		};


		class OutputConnectivity {
		private:
			friend Element;
			OutputConnectivity(CircuitGraph& graph, element_id_t element_id, connection_id_t output_id) :
				graph_(graph),
				element_id_(element_id),
				output_id_(output_id)
			{
			}
		public:
			element_id_t element_id() const {
				return graph_.outputs_.at(output_id_).element_id;
			}

			bool has_element() const {
				return element_id() != null_element;
			}

			Element element() {
				return Element{ graph_, element_id() };
			}

			connection_size_t input_index() const {
				return graph_.outputs_.at(output_id_).index;
			}
			

		private:

			CircuitGraph& graph_;
			element_id_t element_id_;
			connection_id_t output_id_;
		};


		class Element {
		private:
			friend CircuitGraph;
			Element(CircuitGraph& graph, element_id_t element_id) :
				graph_(graph),
				element_id_(element_id)
			{
				if (element_id < 0 || element_id >= graph.element_count()) {
					logicsim::throw_exception("Element id is invalid");
				}
			}

		public:

			element_id_t element_id() {
				return element_id_;
			}

			ElementType type() const
			{
				return element_data_().type;
			}

			connection_size_t input_count() const
			{
				return element_data_().input_count;
			}

			connection_size_t output_count() const
			{
				return element_data_().output_count;
			}

			connection_id_t first_input_id() const {
				return element_data_().first_input_id;
			}

			connection_id_t input_id(connection_size_t input) const {
				if (input < 0 || input >= input_count()) {
					logicsim::throw_exception("Index is invalid");
				}
				return first_input_id() + input;
			}

			connection_id_t first_output_id() const {
				return element_data_().first_output_id;
			}

			connection_id_t output_id(connection_size_t output) const {
				if (output < 0 || output >= output_count()) {
					logicsim::throw_exception("Index is invalid");
				}
				return first_output_id() + output;
			}

			InputConnectivity input(connection_size_t input) {
				return InputConnectivity{graph_, element_id_, input_id(input)};
			}

			OutputConnectivity output(connection_size_t output) {
				return OutputConnectivity{ graph_, element_id_, output_id(output) };
			}

			/*
			void connect_output(
				element_id_t from_element,
				connection_size_t from_output,
				element_id_t to_element = null_element,
				connection_size_t to_input = null_connection)
			{
				ConnectivityData& from_con = get_output_con(element_data_(from_element), from_output);
				ConnectivityData& to_con = get_input_con(element_data_(to_element), to_input);

				from_con.element = to_element;
				from_con.index = to_input;

				to_con.element = from_element;
				to_con.index = from_output;
			}
			*/

		private:
			ElementData& element_data_() {
				return graph_.elements_.at(element_id_);
			}

			const ElementData& element_data_() const
			{
				return graph_.elements_.at(element_id_);
			}


			CircuitGraph& graph_;
			element_id_t element_id_;
		};

		element_id_t element_count() const noexcept
		{
			return static_cast<element_id_t>(elements_.size());
		}

		Element element(element_id_t element_id) {
			return Element{ *this, element_id };
		}

		Element create_element(
			ElementType type,
			connection_size_t input_count,
			connection_size_t output_count)
		{
			elements_.push_back({ static_cast<int>(inputs_.size()), static_cast<int>(outputs_.size()), input_count, output_count, type });
			inputs_.resize(inputs_.size() + input_count);
			outputs_.resize(outputs_.size() + output_count);

			element_id_t element_id = static_cast<element_id_t>(elements_.size() - 1);
			return element(element_id);
		}

		auto total_inputs() const noexcept {
			return inputs_.size();
		}

		auto total_outputs() const noexcept {
			return outputs_.size();
		}

	};




}

#endif