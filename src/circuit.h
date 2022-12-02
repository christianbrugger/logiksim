#ifndef LOGIKSIM_CIRCUIT_H
#define LOGIKSIM_CIRCUIT_H

#include "exceptions.h"

#include <cstdint>
#include <vector>
#include <iostream>
#include <functional>

#include <range/v3/core.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/algorithm/all_of.hpp>


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
	using connection_id_t = int32_t;
	using connection_size_t = int8_t;

	constexpr element_id_t null_element = -1;
	constexpr connection_size_t null_connection = -1;


	class Circuit {
		struct ElementData;
		struct ConnectionData;

	public:
		template<bool Const>
		class ElementTemplate;
		template<bool Const>
		class InputConnectionTemplate;
		template<bool Const>
		class OutputConnectionTemplate;

		using Element = ElementTemplate<false>;
		using ConstElement = ElementTemplate<true>;
		using InputConnection = InputConnectionTemplate<false>;
		using ConstInputConnection = InputConnectionTemplate<true>;
		using OutputConnection = OutputConnectionTemplate<false>;
		using ConstOutputConnection = OutputConnectionTemplate<true>;

	public:
		element_id_t element_count() const noexcept;
		connection_id_t total_input_count() const noexcept;
		connection_id_t total_output_count() const noexcept;

		[[ nodiscard ]] Element element(element_id_t element_id);
		[[ nodiscard ]] ConstElement element(element_id_t element_id) const;
		inline auto elements();
		inline auto elements() const;

		Element create_element(ElementType type,
			connection_size_t input_count, connection_size_t output_count);


		void validate(bool require_all_outputs_connected = false) const;

	private:
		static void validate_connection_data_(Circuit::ConnectionData connection_data);

		std::vector<ElementData> element_data_store_;
		std::vector<ConnectionData> output_data_store_;
		std::vector<ConnectionData> input_data_store_;
	};


	struct Circuit::ElementData {
		connection_id_t first_input_id;
		connection_id_t first_output_id;

		connection_size_t input_count;
		connection_size_t output_count;

		ElementType type;
	};


	struct Circuit::ConnectionData {
		element_id_t element_id { null_element };
		connection_size_t index { null_connection };
	};


	template<bool Const>
	class Circuit::ElementTemplate {
		using CircuitType = std::conditional_t<Const, Circuit const, Circuit>;
		using ElementDataType = std::conditional_t<Const, ElementData const, ElementData>;

		friend Circuit;
		explicit ElementTemplate(CircuitType* circuit, element_id_t element_id);
	public:
		template<bool ConstOther>
		bool operator==(ElementTemplate<ConstOther> other) const noexcept;

		CircuitType* circuit() const noexcept;
		element_id_t element_id() const noexcept;

		ElementType element_type() const;
		connection_size_t input_count() const;
		connection_size_t output_count() const;
		connection_id_t first_input_id() const;
		connection_id_t input_id(connection_size_t input_index) const;
		connection_id_t first_output_id() const;
		connection_id_t output_id(connection_size_t output_index) const;

		[[ nodiscard ]] InputConnectionTemplate<Const> input(connection_size_t input) const;
		[[ nodiscard ]] OutputConnectionTemplate<Const> output(connection_size_t output) const;

		auto inputs() const;
		auto outputs() const;

	private:
		ElementDataType& element_data_() const;

		CircuitType* circuit_;
		element_id_t element_id_;
	};


	template<bool Const>
	class Circuit::InputConnectionTemplate {
		using CircuitType = std::conditional_t<Const, Circuit const, Circuit>;
		using ConnectionDataType = std::conditional_t<Const, ConnectionData const, ConnectionData>;

		friend ElementTemplate<Const>;
		explicit InputConnectionTemplate(CircuitType* circuit, element_id_t element_id,
			connection_size_t input_index, connection_id_t input_id);
	public:
		template<bool ConstOther>
		bool operator==(InputConnectionTemplate<ConstOther> other) const noexcept;

		CircuitType* circuit() const noexcept;
		element_id_t element_id() const noexcept;
		connection_size_t input_index() const noexcept;
		connection_id_t input_id() const noexcept;

		ElementTemplate<Const> element() const;
		bool has_connected_element() const;
		element_id_t connected_element_id() const;
		connection_size_t connected_output_index() const;


		/// @throws if connection doesn't exists. Call has_connected_element to check for this.
		[[ nodiscard ]] ElementTemplate<Const> connected_element() const;
		/// @throws if connection doesn't exists. Call has_connected_element to check for this.
		[[ nodiscard ]] OutputConnectionTemplate<Const> connected_output() const;

		void clear_connection() const;
		template<bool ConstOther>
		void connect(OutputConnectionTemplate<ConstOther> output) const;
	private:
		ConnectionDataType& connection_data_() const;

		CircuitType* circuit_;
		element_id_t element_id_;
		connection_size_t input_index_;
		connection_id_t input_id_;
	};

	template<bool Const>
	class Circuit::OutputConnectionTemplate {
	private:
		using CircuitType = std::conditional_t<Const, Circuit const, Circuit>;
		using ConnectionDataType = std::conditional_t<Const, ConnectionData const, ConnectionData>;

		friend ElementTemplate<Const>;
		explicit OutputConnectionTemplate(CircuitType* circuit, element_id_t element_id,
			connection_size_t output_index, connection_id_t output_id
		);
	public:
		template<bool ConstOther>
		bool operator==(OutputConnectionTemplate<ConstOther> other) const noexcept;

		CircuitType* circuit() const noexcept;
		element_id_t element_id() const noexcept;
		connection_size_t output_index() const noexcept;
		connection_id_t output_id() const noexcept;

		ElementTemplate<Const> element() const;
		[[nodiscard]] bool has_connected_element() const;
		element_id_t connected_element_id() const;
		connection_size_t connected_input_index() const;

		/// @throws if connection doesn't exists. Call has_connected_element to check for this.
		[[nodiscard]] ElementTemplate<Const> connected_element() const;
		/// @throws if connection doesn't exists. Call has_connected_element to check for this.
		[[nodiscard]] InputConnectionTemplate<Const> connected_input() const;

		void clear_connection() const;
		template<bool ConstOther>
		void connect(InputConnectionTemplate<ConstOther> input) const;
	private:
		ConnectionDataType& connection_data_() const;

		CircuitType* circuit_;
		element_id_t element_id_;
		connection_size_t output_index_;
		connection_id_t output_id_;
	};



	void create_placeholders(Circuit& circuit);

	Circuit benchmark_circuit(const int n_elements = 100);

	//
	// Circuit
	//

	// auto return methods need to be defined in the header, so the type can be deduced

	auto Circuit::elements() {
		return ranges::views::iota(0, element_count()) | ranges::views::transform(
			[this](int i) { return this->element(static_cast<element_id_t>(i)); });
	}

	auto Circuit::elements() const {
		return ranges::views::iota(0, element_count()) | ranges::views::transform(
			[this](int i) { return this->element(static_cast<element_id_t>(i)); });
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
	template<bool ConstOther>
	bool Circuit::ElementTemplate<Const>::operator==(ElementTemplate<ConstOther> other) const noexcept
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
	auto Circuit::ElementTemplate<Const>::input(connection_size_t input) const -> InputConnectionTemplate<Const>
	{
		return InputConnectionTemplate<Const>{ circuit_, element_id_, input, input_id(input) };
	}

	template<bool Const>
	auto Circuit::ElementTemplate<Const>::output(connection_size_t output) const -> OutputConnectionTemplate<Const>
	{
		return OutputConnectionTemplate<Const>{ circuit_, element_id_, output, output_id(output) };
	}

	template<bool Const>
	inline auto Circuit::ElementTemplate<Const>::inputs() const {
		return ranges::views::iota(0, input_count()) | ranges::views::transform(
			[this](int i) { return this->input(static_cast<connection_size_t>(i)); });
	}

	template<bool Const>
	inline auto Circuit::ElementTemplate<Const>::outputs() const {
		return ranges::views::iota(0, output_count()) | ranges::views::transform(
			[this](int i) { return this->output(static_cast<connection_size_t>(i)); });
	}

	template<bool Const>
	auto Circuit::ElementTemplate<Const>::element_data_() const -> ElementDataType&
	{
		return circuit_->element_data_store_.at(element_id_);
	}


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
	template<bool ConstOther>
	bool Circuit::InputConnectionTemplate<Const>::operator==(InputConnectionTemplate<ConstOther> other) const noexcept
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
		return ElementTemplate<Const> { circuit_, connected_element_id() };
	}

	template<bool Const>
	auto Circuit::InputConnectionTemplate<Const>::connected_output() const -> OutputConnectionTemplate<Const>
	{
		return connected_element().output(connected_output_index());
	}

	template<bool Const>
	void Circuit::InputConnectionTemplate<Const>::clear_connection() const
	{
		static_assert(!Const, "Cannot clear connection for const circuit.");

		auto& connection_data { connection_data_() };
		if (connection_data.element_id != null_element) {
			auto& destination_connection_data { circuit_->output_data_store_.at(
				circuit_->element(connection_data.index).output_id(connection_data.index)) };

			destination_connection_data.element_id = null_element;
			destination_connection_data.index = null_connection;

			connection_data.element_id = null_element;
			connection_data.index = null_connection;
		}
	}

	template<bool Const>
	template<bool ConstOther>
	void Circuit::InputConnectionTemplate<Const>::connect(OutputConnectionTemplate< ConstOther> output) const
	{
		static_assert(!Const, "Cannot connect input for const circuit.");
		clear_connection();

		// get data before we modify anything, for exception safety
		auto& destination_connection_data { circuit_->output_data_store_.at(output.output_id()) };
		auto& connection_data { connection_data_() };

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
	template<bool ConstOther>
	bool Circuit::OutputConnectionTemplate<Const>::operator==(Circuit::OutputConnectionTemplate<ConstOther> other) const noexcept
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

	template<bool Const>
	void Circuit::OutputConnectionTemplate<Const>::clear_connection() const {
		static_assert(!Const, "Cannot clear connection for const circuit.");

		auto& connection_data { connection_data_() };
		if (connection_data.element_id != null_element) {
			auto& destination_connection_data = circuit_->input_data_store_.at(
				circuit_->element(connection_data.index).input_id(connection_data.index));
			destination_connection_data.element_id = null_element;
			destination_connection_data.index = null_connection;

			connection_data.element_id = null_element;
			connection_data.index = null_connection;
		}
	}

	template<bool Const>
	template<bool ConstOther>
	void Circuit::OutputConnectionTemplate<Const>::connect(InputConnectionTemplate<ConstOther> input) const
	{
		static_assert(!Const, "Cannot connect output for const circuit.");
		clear_connection();

		// get data before we modify anything, for exception safety
		auto& connection_data { connection_data_() };
		auto& destination_connection_data { circuit_->input_data_store_.at(input.input_id()) };

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


}




#endif