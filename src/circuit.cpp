
#include "circuit.h"

#include <ranges>
#include <algorithm>


namespace logicsim {

	//
	// Circuit
	//

	element_id_t Circuit::element_count() const noexcept
	{
		return static_cast<element_id_t>(element_data_store_.size());
	}

	auto Circuit::element(element_id_t element_id) -> Element
	{
		return Element { this, element_id };
	}

	auto Circuit::element(element_id_t element_id) const -> ConstElement
	{
		return ConstElement { this, element_id };
	}

	auto Circuit::create_element(ElementType type,
		connection_size_t input_count, connection_size_t output_count) -> Element
	{
		if (input_count < 0) {
			throw_exception("Input count needs to be positive.");
		}
		if (output_count < 0) {
			throw_exception("Output count needs to be positive.");
		}

		const auto new_input_size = input_data_store_.size() + input_count;
		const auto new_output_size = input_data_store_.size() + input_count;

		// make sure we can represent all ids
		if (element_data_store_.size() + 1 >= std::numeric_limits<element_id_t>::max()) {
			throw_exception("Reached maximum number of elements.");
		}
		if (new_input_size >= std::numeric_limits<connection_id_t>::max()) {
			throw_exception("Reached maximum number of inputs.");
		}
		if (new_output_size >= std::numeric_limits<connection_id_t>::max()) {
			throw_exception("Reached maximum number of outputs.");
		}
		// TODO create custom exception, as we want to handle theses ones.

		element_data_store_.push_back({
			static_cast<connection_id_t>(input_data_store_.size()),
			static_cast<connection_id_t>(output_data_store_.size()),
			input_count,
			output_count,
			type
			});
		input_data_store_.resize(new_input_size);
		output_data_store_.resize(new_output_size);

		element_id_t element_id = static_cast<element_id_t>(element_data_store_.size() - 1);
		return element(element_id);
	}

	connection_id_t Circuit::total_input_count() const noexcept
	{
		return static_cast<connection_id_t>(input_data_store_.size());
	}

	connection_id_t Circuit::total_output_count() const noexcept
	{
		return static_cast<connection_id_t>(output_data_store_.size());
	}

	void validate_output_connected(const Circuit::ConstOutputConnection output) {
		if (!output.has_connected_element()) {
			throw_exception("Element has unconnected output.");
		}
	}

	void validate_outputs_connected(const Circuit::ConstElement element) {
		std::ranges::for_each(element.outputs(), validate_output_connected);
	}

	void validate_input_consistent(const Circuit::ConstInputConnection input) {
		if (input.has_connected_element()) {
			auto back_reference = input.connected_output().connected_input();
			if (back_reference != input) {
				throw_exception("Back reference doesn't match.");
			}
		}
	}

	void validate_output_consistent(const Circuit::ConstOutputConnection output) {
		if (output.has_connected_element()) {
			auto back_reference = output.connected_input().connected_output();
			if (back_reference != output) {
				throw_exception("Back reference doesn't match.");
			}
		}
	}

	void validate_element_connections_consistent(const Circuit::ConstElement element) {
		std::ranges::for_each(element.inputs(), validate_input_consistent);
		std::ranges::for_each(element.outputs(), validate_output_consistent);
	}

	void Circuit::validate_connection_data_(const Circuit::ConnectionData connection_data)
	{
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

	void Circuit::validate(bool require_all_outputs_connected)  const 
	{
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


	//
	// Circuit::Element
	//

	template<bool Const>
	Circuit::ElementTemplate<Const>::ElementTemplate(CircuitType* circuit, element_id_t element_id) :
		circuit_(circuit),
		element_id_(element_id)
	{
		if (circuit == nullptr) {
			throw_exception("Circuit cannot be nullptr.");
		}
		if (element_id < 0 || element_id >= circuit->element_count()) {
			throw_exception("Element id is invalid");
		}
	}

	template<bool Const>
	bool Circuit::ElementTemplate<Const>::operator==(ElementTemplate<Const> other) const noexcept
	{
		return circuit_ == other.circuit_ &&
			element_id_ == other.element_id_;
	}

	template<bool Const>
	auto Circuit::ElementTemplate<Const>::circuit() const noexcept -> CircuitType*
	{
		return circuit_;
	}

	template<bool Const>
	element_id_t Circuit::ElementTemplate<Const>::element_id() const noexcept
	{
		return element_id_;
	}

	template<bool Const>
	ElementType Circuit::ElementTemplate<Const>::element_type() const
	{
		return element_data_().type;
	}

	template<bool Const>
	connection_size_t Circuit::ElementTemplate<Const>::input_count() const
	{
		return element_data_().input_count;
	}

	template<bool Const>
	connection_size_t Circuit::ElementTemplate<Const>::output_count() const
	{
		return element_data_().output_count;
	}

	template<bool Const>
	connection_id_t Circuit::ElementTemplate<Const>::first_input_id() const
	{
		return element_data_().first_input_id;
	}

	template<bool Const>
	connection_id_t Circuit::ElementTemplate<Const>::input_id(connection_size_t input_index) const
	{
		if (input_index < 0 || input_index >= input_count()) {
			throw_exception("Index is invalid");
		}
		return first_input_id() + input_index;
	}

	template<bool Const>
	connection_id_t Circuit::ElementTemplate<Const>::first_output_id() const
	{
		return element_data_().first_output_id;
	}

	template<bool Const>
	connection_id_t Circuit::ElementTemplate<Const>::output_id(connection_size_t output_index) const
	{
		if (output_index < 0 || output_index >= output_count()) {
			throw_exception("Index is invalid");
		}
		return first_output_id() + output_index;
	}


	template<bool Const>
	auto Circuit::ElementTemplate<Const>::element_data_() const -> ElementDataType&
	{
		return circuit_->element_data_store_.at(element_id_);
	}

	template class Circuit::ElementTemplate<true>;
	template class Circuit::ElementTemplate<false>;

	//
	// Circuit::InputConnection
	//


	template<bool Const>
	Circuit::InputConnectionTemplate<Const>::InputConnectionTemplate(CircuitType* circuit,
		element_id_t element_id, connection_size_t input_index, connection_id_t input_id
	) :
		circuit_(circuit),
		element_id_(element_id),
		input_index_(input_index),
		input_id_(input_id)
	{
		if (circuit == nullptr) {
			throw_exception("Circuit cannot be nullptr.");
		}
	}

	template<bool Const>
	bool Circuit::InputConnectionTemplate<Const>::operator==(InputConnectionTemplate<Const> other) const noexcept
	{
		return circuit_ == other.circuit_ &&
			element_id_ == other.element_id_ &&
			input_index_ == other.input_index_ &&
			input_index_ == other.input_index_;
	}

	template<bool Const>
	auto Circuit::InputConnectionTemplate<Const>::circuit() const noexcept -> CircuitType*
	{
		return circuit_;
	}

	template<bool Const>
	element_id_t Circuit::InputConnectionTemplate<Const>::element_id() const noexcept
	{
		return element_id_;
	}

	template<bool Const>
	connection_size_t Circuit::InputConnectionTemplate<Const>::input_index() const noexcept
	{
		return input_index_;
	}

	template<bool Const>
	connection_id_t Circuit::InputConnectionTemplate<Const>::input_id() const noexcept
	{
		return input_id_;
	}

	template<bool Const>
	auto Circuit::InputConnectionTemplate<Const>::element() const -> ElementTemplate<Const>
	{
		return ElementTemplate<Const>{ circuit_, element_id_ };
	}

	template<bool Const>
	bool Circuit::InputConnectionTemplate<Const>::has_connected_element() const
	{
		return connected_element_id() != null_element;
	}

	template<bool Const>
	element_id_t Circuit::InputConnectionTemplate<Const>::connected_element_id() const
	{
		return connection_data_().element_id;
	}

	template<bool Const>
	connection_size_t Circuit::InputConnectionTemplate<Const>::connected_output_index() const
	{
		return connection_data_().index;
	}

	template<bool Const>
	auto Circuit::InputConnectionTemplate<Const>::connected_element() const -> ElementTemplate<Const>
	{
		// TODO remove ElementTemplate<Const> and try with explicit
		return ElementTemplate<Const>{ circuit_, connected_element_id() };
	}

	template<bool Const>
	auto Circuit::InputConnectionTemplate<Const>::connected_output() const -> OutputConnectionTemplate<Const>
	{
		return connected_element().output(connected_output_index());
	}

	// TODO experiment with static assert
	template<>
	void Circuit::InputConnectionTemplate<false>::clear_connection() const
	{
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

	// TODO experiment with static assert
	template<>
	void Circuit::InputConnectionTemplate<false>::connect(OutputConnection output) const
	{
		clear_connection();

		// get data before we modify anything
		auto& destination_connection_data = circuit_->output_data_store_.at(output.output_id());
		auto& connection_data = connection_data_();

		connection_data.element_id = output.element_id();
		connection_data.index = output.output_index();

		destination_connection_data.element_id = element_id();
		destination_connection_data.index = input_index();
	}

	template<bool Const>
	auto Circuit::InputConnectionTemplate<Const>::connection_data_() const -> ConnectionDataType&
	{
		return circuit_->input_data_store_.at(input_id_);
	}

	template class Circuit::InputConnectionTemplate<true>;
	template class Circuit::InputConnectionTemplate<false>;

	//
	// Circuit::OutputConnection
	//


	template<bool Const>
	Circuit::OutputConnectionTemplate<Const>::OutputConnectionTemplate(
		CircuitType* circuit,
		element_id_t element_id,
		connection_size_t output_index,
		connection_id_t output_id
	) :
		circuit_(circuit),
		element_id_(element_id),
		output_index_(output_index),
		output_id_(output_id)
	{
		if (circuit == nullptr) {
			throw_exception("Circuit cannot be nullptr.");
		}
	}

	template<bool Const>
	bool Circuit::OutputConnectionTemplate<Const>::operator==(Circuit::OutputConnectionTemplate<Const> other) const noexcept
	{
		return circuit_ == other.circuit_ &&
			element_id_ == other.element_id_ &&
			output_index_ == other.output_index_ &&
			output_id_ == other.output_id_;
	}

	template<bool Const>
	auto Circuit::OutputConnectionTemplate<Const>::circuit() const noexcept -> CircuitType*
	{
		return circuit_;
	}

	template<bool Const>
	element_id_t Circuit::OutputConnectionTemplate<Const>::element_id() const noexcept
	{
		return element_id_;
	}

	template<bool Const>
	connection_size_t Circuit::OutputConnectionTemplate<Const>::output_index() const noexcept
	{
		return output_index_;
	}

	template<bool Const>
	connection_id_t Circuit::OutputConnectionTemplate<Const>::output_id() const noexcept
	{
		return output_id_;
	}

	template<bool Const>
	auto Circuit::OutputConnectionTemplate<Const>::element() const -> ElementTemplate<Const>
	{
		return Circuit::ElementTemplate<Const>{ circuit_, element_id_ };
	}

	template<bool Const>
	bool Circuit::OutputConnectionTemplate<Const>::has_connected_element() const
	{
		return connected_element_id() != null_element;
	}

	template<bool Const>
	element_id_t Circuit::OutputConnectionTemplate<Const>::connected_element_id() const
	{
		return connection_data_().element_id;
	}

	template<bool Const>
	connection_size_t Circuit::OutputConnectionTemplate<Const>::connected_input_index() const
	{
		return connection_data_().index;
	}

	template<bool Const>
	auto Circuit::OutputConnectionTemplate<Const>::connected_element() const -> ElementTemplate<Const>
	{
		return ElementTemplate<Const>{ circuit_, connected_element_id() };
	}

	template<bool Const>
	auto Circuit::OutputConnectionTemplate<Const>::connected_input() const -> InputConnectionTemplate<Const>
	{
		return connected_element().input(connected_input_index());
	}

	template<>
	void Circuit::OutputConnectionTemplate<false>::clear_connection() const {
		auto& connection_data = connection_data_();

		if (connection_data.element_id != null_element) {
			auto& destination_connection_data = circuit_->input_data_store_.at(
				circuit_->element(connection_data.index).input_id(connection_data.index));
			destination_connection_data.element_id = null_element;
			destination_connection_data.index = null_connection;

			connection_data.element_id = null_element;
			connection_data.index = null_connection;
		}
	}

	template<>
	void Circuit::OutputConnectionTemplate<false>::connect(InputConnection input) const
	{
		clear_connection();

		// get data before we modify anything
		auto& connection_data = connection_data_();
		auto& destination_connection_data = circuit_->input_data_store_.at(input.input_id());

		connection_data.element_id = input.element_id();
		connection_data.index = input.input_index();

		destination_connection_data.element_id = element_id();
		destination_connection_data.index = output_index();
	}

	template<bool Const>
	auto Circuit::OutputConnectionTemplate<Const>::connection_data_() const -> ConnectionDataType&
	{
		return circuit_->output_data_store_.at(output_id_);
	}

	template class Circuit::OutputConnectionTemplate<true>;
	template class Circuit::OutputConnectionTemplate<false>;

	//
	// Free Functions
	//


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

	void create_placeholders(Circuit& circuit) {
		std::ranges::for_each(circuit.elements(), create_element_placeholders);
	}




	Circuit benchmark_circuit(const int n_elements) {

		Circuit circuit {};

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
