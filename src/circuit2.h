#ifndef LOGIKSIM_CIRCUIT_2_H
#define LOGIKSIM_CIRCUIT_2_H

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


	class Circuit {

	private:
		struct ConnectionData {
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
		std::vector<ConnectionData> outputs_;
		std::vector<ConnectionData> inputs_;

	public:
		class Element;


		class InputConnection {
		private:
			friend Element;
			InputConnection(Circuit *circuit, element_id_t element_id, connection_size_t input_index, connection_id_t input_id) :
				circuit_(circuit),
				element_id_(element_id),
				input_index_(input_index),
				input_id_(input_id)
			{
				if (circuit == nullptr) {
					logicsim::throw_exception("Circuit cannot be nullptr.");
				}
			}
		public:
			element_id_t element_id() const {
				return element_id_;
			}

			connection_size_t index() const {
				return input_index_;
			}

			connection_id_t id() const {
				return input_id_;
			}

			element_id_t connected_element_id() const {
				return connection_data_().element_id;
			}

			bool has_connected_element() const {
				return connected_element_id() != null_element;
			}

			Element connected_element() {
				return Element{ circuit_, connected_element_id() };
			}

			connection_size_t connected_output_index() const {
				return connection_data_().index;
			}

			void connect(InputConnection& output) {
				clear_connection();

				auto& connection_data = connection_data_();
				connection_data.element_id = output.element_id();
				connection_data.index = output.index();

				auto& destination_connection_data = circuit_->outputs_.at(output.id());
				destination_connection_data.element_id = element_id();
				destination_connection_data.index = index();
			}

			void clear_connection() {
				auto& connection_data = connection_data_();

				if (connection_data.element_id != null_element) {
					auto& destination_connection_data = circuit_->outputs_.at(
						circuit_->element(connection_data.index).output_id(connection_data.index));
					destination_connection_data.element_id = null_element;
					destination_connection_data.index = null_connection;

					connection_data.element_id = null_element;
					connection_data.index = null_connection;
				}
			}

		private:
			ConnectionData& connection_data_() {
				return circuit_->inputs_.at(input_id_);
			}

			const ConnectionData& connection_data_() const {
				return circuit_->inputs_.at(input_id_);
			}

			Circuit *circuit_;
			element_id_t element_id_;
			connection_size_t input_index_;
			connection_id_t input_id_;
		};


		class OutputConnection {
		private:
			friend Element;
			OutputConnection(Circuit *circuit, element_id_t element_id, connection_size_t output_index, connection_id_t output_id) :
				circuit_(circuit),
				element_id_(element_id),
				output_index_(output_index),
				output_id_(output_id)
			{
				if (circuit == nullptr) {
					logicsim::throw_exception("Circuit cannot be nullptr.");
				}
			}
		public:
			element_id_t element_id() const {
				return element_id_;
			}

			connection_size_t index() const {
				return output_index_;
			}

			connection_id_t id() const {
				return output_id_;
			}

			element_id_t connected_element_id() const {
				return connection_data_().element_id;
			}

			bool has_connected_element() const {
				return connected_element_id() != null_element;
			}

			Element connected_element() {
				return Element{ circuit_, connected_element_id() };
			}

			connection_size_t connected_input_index() const {
				return connection_data_().index;
			}

			void connect(const InputConnection &input) {
				clear_connection();

				auto& connection_data = connection_data_();
				connection_data.element_id = input.element_id();
				connection_data.index = input.index();

				auto& destination_connection_data = circuit_->inputs_.at(input.id());
				destination_connection_data.element_id = element_id();
				destination_connection_data.index = index();
			}

			void clear_connection() {
				auto &connection_data = connection_data_();
				
				if (connection_data.element_id != null_element) {
					auto& destination_connection_data = circuit_->inputs_.at(
						circuit_->element(connection_data.index).input_id(connection_data.index));
					destination_connection_data.element_id = null_element;
					destination_connection_data.index = null_connection;

					connection_data.element_id = null_element;
					connection_data.index = null_connection;
				}
			}

		private:
			ConnectionData& connection_data_() {
				return circuit_->outputs_.at(output_id_);
			}

			const ConnectionData& connection_data_() const {
				return circuit_->outputs_.at(output_id_);
			}

			Circuit *circuit_;
			element_id_t element_id_;
			connection_size_t output_index_;
			connection_id_t output_id_;
		};


		class Element {
		private:
			friend Circuit;
			Element(Circuit *circuit, element_id_t element_id) :
				circuit_(circuit),
				element_id_(element_id)
			{
				if (circuit == nullptr) {
					logicsim::throw_exception("Circuit cannot be nullptr.");
				}
				if (element_id < 0 || element_id >= circuit->element_count()) {
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

			InputConnection input(connection_size_t input) {
				return InputConnection { circuit_, element_id_, input, input_id(input) };
			}

			OutputConnection output(connection_size_t output) {
				return OutputConnection { circuit_, element_id_, output, output_id(output) };
			}

		private:
			ElementData& element_data_() {
				return circuit_->elements_.at(element_id_);
			}

			const ElementData& element_data_() const
			{
				return circuit_->elements_.at(element_id_);
			}

			Circuit *circuit_;
			element_id_t element_id_;
		};

		// -----------------

		element_id_t element_count() const noexcept
		{
			return static_cast<element_id_t>(elements_.size());
		}

		Element element(element_id_t element_id) {
			return Element{ this, element_id };
		}

		Element create_element(
			ElementType type,
			connection_size_t input_count,
			connection_size_t output_count)
		{
			elements_.push_back({
				static_cast<connection_id_t>(inputs_.size()), 
				static_cast<connection_id_t>(outputs_.size()), 
				input_count, 
				output_count, 
				type
			});
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


	Circuit benchmark_circuit(const int n_elements = 100);

}

#endif