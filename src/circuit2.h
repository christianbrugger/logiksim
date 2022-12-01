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
		struct ElementData {
			connection_id_t first_input_id;
			connection_id_t first_output_id;

			connection_size_t input_count;
			connection_size_t output_count;

			ElementType type;
		};

		struct ConnectionData {
			element_id_t element_id = null_element;
			connection_size_t index = null_connection;
		};

		std::vector<ElementData> element_data_store_;
		std::vector<ConnectionData> output_data_store_;
		std::vector<ConnectionData> input_data_store_;

	public:
		class Element;
		class InputConnection;
		class OutputConnection;


		class Element {
		private:
			friend Circuit;
			Element(Circuit* circuit, element_id_t element_id) :
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
			bool operator==(Element other) {
				return circuit_ == other.circuit_ &&
					element_id_ == other.element_id_;
			}

			Circuit* circuit() {
				return circuit_;
			}

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
				return InputConnection{ circuit_, element_id_, input, input_id(input) };
			}

			OutputConnection output(connection_size_t output) {
				return OutputConnection{ circuit_, element_id_, output, output_id(output) };
			}

			auto inputs() {
				return std::views::iota(0, input_count()) | std::views::transform(
					[this](int i) { return input(static_cast<connection_size_t>(i)); });
			}

			auto outputs() {
				return std::views::iota(0, output_count()) | std::views::transform(
					[this](int i) { return output(static_cast<connection_size_t>(i)); });
			}

		private:
			ElementData& element_data_() {
				return circuit_->element_data_store_.at(element_id_);
			}

			const ElementData& element_data_() const
			{
				return circuit_->element_data_store_.at(element_id_);
			}

			Circuit* circuit_;
			element_id_t element_id_;
		};


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
			bool operator==(InputConnection other) {
				return circuit_ == other.circuit_ &&
					element_id_ == other.element_id_ &&
					input_index_ == other.input_index_ &&
					input_index_ == other.input_index_;
			}

			Circuit* circuit() {
				return circuit_;
			}

			element_id_t element_id() const {
				return element_id_;
			}

			connection_size_t input_index() const {
				return input_index_;
			}

			connection_id_t input_id() const {
				return input_id_;
			}

			Element element() {
				return Element{ circuit_, element_id_ };
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

			OutputConnection connected_output() {
				return connected_element().output(connected_output_index());
			}

			void connect(OutputConnection output) {
				clear_connection();

				auto& connection_data = connection_data_();
				connection_data.element_id = output.element_id();
				connection_data.index = output.output_index();

				auto& destination_connection_data = circuit_->output_data_store_.at(output.output_id());
				destination_connection_data.element_id = element_id();
				destination_connection_data.index = input_index();
			}

			void clear_connection() {
				auto& connection_data = connection_data_();

				if (connection_data.element_id != null_element) {
					auto& destination_connection_data = circuit_->output_data_store_.at(
						circuit_->element(connection_data.index).output_id(connection_data.index));
					destination_connection_data.element_id = null_element;
					destination_connection_data.index = null_connection;

					connection_data.element_id = null_element;
					connection_data.index = null_connection;
				}
			}

		private:
			ConnectionData& connection_data_() {
				return circuit_->input_data_store_.at(input_id_);
			}

			const ConnectionData& connection_data_() const {
				return circuit_->input_data_store_.at(input_id_);
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
			bool operator==(OutputConnection other) {
				return circuit_ == other.circuit_ &&
					element_id_ == other.element_id_ &&
					output_index_ == other.output_index_ &&
					output_id_ == other.output_id_;
			}

			Circuit* circuit() {
				return circuit_;
			}

			element_id_t element_id() const {
				return element_id_;
			}

			connection_size_t output_index() const {
				return output_index_;
			}

			connection_id_t output_id() const {
				return output_id_;
			}

			Element element() {
				return Element{ circuit_, element_id_ };
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

			InputConnection connected_input() {
				return connected_element().input(connected_input_index());
			}

			void connect(InputConnection input) {
				clear_connection();

				auto& connection_data = connection_data_();
				connection_data.element_id = input.element_id();
				connection_data.index = input.input_index();

				auto& destination_connection_data = circuit_->input_data_store_.at(input.input_id());
				destination_connection_data.element_id = element_id();
				destination_connection_data.index = output_index();
			}

			void clear_connection() {
				auto &connection_data = connection_data_();
				
				if (connection_data.element_id != null_element) {
					auto& destination_connection_data = circuit_->input_data_store_.at(
						circuit_->element(connection_data.index).input_id(connection_data.index));
					destination_connection_data.element_id = null_element;
					destination_connection_data.index = null_connection;

					connection_data.element_id = null_element;
					connection_data.index = null_connection;
				}
			}

		private:
			ConnectionData& connection_data_() {
				return circuit_->output_data_store_.at(output_id_);
			}

			const ConnectionData& connection_data_() const {
				return circuit_->output_data_store_.at(output_id_);
			}

			Circuit *circuit_;
			element_id_t element_id_;
			connection_size_t output_index_;
			connection_id_t output_id_;
		};

		// -----------------

		element_id_t element_count() const noexcept
		{
			return static_cast<element_id_t>(element_data_store_.size());
		}

		Element element(element_id_t element_id) {
			return Element{ this, element_id };
		}

		auto elements() {
			return std::views::iota(0, element_count()) | std::views::transform(
				[this](int i) { return this->element(static_cast<element_id_t>(i)); });
		}

		Element create_element(
			ElementType type,
			connection_size_t input_count,
			connection_size_t output_count
		)
		{
			element_data_store_.push_back({
				static_cast<connection_id_t>(input_data_store_.size()),
				static_cast<connection_id_t>(output_data_store_.size()),
				input_count, 
				output_count, 
				type
			});
			input_data_store_.resize(input_data_store_.size() + input_count);
			output_data_store_.resize(output_data_store_.size() + output_count);

			element_id_t element_id = static_cast<element_id_t>(element_data_store_.size() - 1);
			return element(element_id);
		}

		auto total_input_count() const noexcept {
			return input_data_store_.size();
		}

		auto total_output_count() const noexcept {
			return output_data_store_.size();
		}

		void validate(bool require_all_outputs_connected = false);

	private:
		static void validate_connection_data_(Circuit::ConnectionData connection_data);
	};

	void create_placeholders(Circuit &circuit);

	Circuit benchmark_circuit(const int n_elements = 100);

}

#endif