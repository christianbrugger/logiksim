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
		class InputConnection;
		class OutputConnection;

		using Element = ElementTemplate<false>;
		using ConstElement = ElementTemplate<true>;

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

		
		void validate(bool require_all_outputs_connected = false); // TODO find a way to add const

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
		element_id_t element_id{ null_element };
		connection_size_t index{ null_connection };
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

		[[ nodiscard ]] InputConnection input(connection_size_t input) const;
		[[ nodiscard ]] OutputConnection output(connection_size_t output) const;

		auto inputs() const;
		auto outputs() const;

	private:
		ElementDataType& element_data_() const;

		CircuitType* circuit_;
		element_id_t element_id_;
	};


	class Circuit::InputConnection {
	private:
		friend Element;
		explicit InputConnection(Circuit* circuit, element_id_t element_id,
			connection_size_t input_index, connection_id_t input_id);
	public:
		bool operator==(InputConnection other) const noexcept;

		Circuit* circuit() const noexcept;
		element_id_t element_id() const noexcept;
		connection_size_t input_index() const noexcept;
		connection_id_t input_id() const noexcept;

		Element element() const;
		bool has_connected_element() const;
		element_id_t connected_element_id() const;
		connection_size_t connected_output_index() const;

		
		///
		// Returns connected element object.
		//
		// @throws if connection doesn't exists. Call has_connected_element to check for this.
		//
		[[ nodiscard ]] Element connected_element() const;

		///
		// Returns connected output object.
		//
		// @throws if connection doesn't exists. Call has_connected_element to check for this.
		//
		[[ nodiscard ]] OutputConnection connected_output() const;

		void connect(OutputConnection output) const;
		void clear_connection() const;
	private:
		ConnectionData& connection_data_() const;

		Circuit* circuit_;
		element_id_t element_id_;
		connection_size_t input_index_;
		connection_id_t input_id_;
	};

	class Circuit::OutputConnection {
	private:
		friend Element;
		explicit OutputConnection(Circuit* circuit, element_id_t element_id,
			connection_size_t output_index, connection_id_t output_id
		);
	public:
		bool operator==(OutputConnection other) const noexcept;

		Circuit* circuit() const noexcept;
		element_id_t element_id() const noexcept;
		connection_size_t output_index() const noexcept;
		connection_id_t output_id() const noexcept;

		Element element() const;
		[[nodiscard]] bool has_connected_element() const;
		element_id_t connected_element_id() const;
		connection_size_t connected_input_index() const;

		/**
		 * Returns connected element object.
		 *
		 * @throws if connection doesn't exists. Call has_connected_element to check for this.
		 */
		[[nodiscard]] Element connected_element() const;
		/**
		 * Returns connected input object.
		 *
		 * @throws if connection doesn't exists. Call has_connected_element to check for this.
		 */
		[[nodiscard]] InputConnection connected_input() const;

		void connect(InputConnection input) const;
		void clear_connection() const;

	private:
		ConnectionData& connection_data_() const;

		Circuit* circuit_;
		element_id_t element_id_;
		connection_size_t output_index_;
		connection_id_t output_id_;
	};



	void create_placeholders(Circuit& circuit);

	Circuit benchmark_circuit(const int n_elements = 100);

	//
	// Circuit
	//

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

	//template<bool Const>
	template<>
	inline Circuit::InputConnection Circuit::ElementTemplate<false>::input(connection_size_t input) const
	{
		return Circuit::InputConnection{ circuit_, element_id_, input, input_id(input) };
	}

	//template<bool Const>
	template<>
	inline Circuit::OutputConnection Circuit::ElementTemplate<false>::output(connection_size_t output) const
	{
		return Circuit::OutputConnection{ circuit_, element_id_, output, output_id(output) };
	}

	// template<bool Const>
	template<>
	inline auto Circuit::ElementTemplate<false>::inputs() const {
		return std::views::iota(0, input_count()) | std::views::transform(
			[this](int i) { return input(static_cast<connection_size_t>(i)); });
	}

	// template<bool Const>
	template<>
	inline auto Circuit::ElementTemplate<false>::outputs() const {
		return std::views::iota(0, output_count()) | std::views::transform(
			[this](int i) { return output(static_cast<connection_size_t>(i)); });
	}


}




#endif