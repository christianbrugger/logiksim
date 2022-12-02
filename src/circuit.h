#ifndef LOGIKSIM_CIRCUIT_H
#define LOGIKSIM_CIRCUIT_H

#include <ranges>
#include <cstdint>
#include <vector>
#include <iostream>
#include <functional>

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
		bool operator==(ElementTemplate other) const noexcept;

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
		bool operator==(InputConnectionTemplate other) const noexcept;

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

		void connect(OutputConnection output) const;
		void clear_connection() const;
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
		bool operator==(OutputConnectionTemplate other) const noexcept;

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

		void connect(InputConnection input) const;
		void clear_connection() const;
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
		return std::views::iota(0, element_count()) | std::views::transform(
			[this](int i) { return this->element(static_cast<element_id_t>(i)); });
	}

	auto Circuit::elements() const {
		return std::views::iota(0, element_count()) | std::views::transform(
			[this](int i) { return this->element(static_cast<element_id_t>(i)); });
	}

	//
	// Circuit::Element
	//

	// auto return methods need to be defined in the header, so the type can be deduced

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
		return std::views::iota(0, input_count()) | std::views::transform(
			[this](int i) { return this->input(static_cast<connection_size_t>(i)); });
	}

	template<bool Const>
	inline auto Circuit::ElementTemplate<Const>::outputs() const {
		return std::views::iota(0, output_count()) | std::views::transform(
			[this](int i) { return this->output(static_cast<connection_size_t>(i)); });
	}


}




#endif