#ifndef LOGIKSIM_CIRCUIT_H
#define LOGIKSIM_CIRCUIT_H

#include "exceptions.h"

#include <fmt/format.h>
#include <range/v3/all.hpp>

#include <cstdint>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace logicsim {

enum class ElementType : uint8_t {
    placeholder,  // has no logic
    wire,
    inverter_element,
    and_element,
    or_element,
    xor_element
};

std::string format(ElementType type);

using element_id_t = int32_t;
using connection_id_t = int32_t;
using connection_size_t = int8_t;

constexpr element_id_t null_element = -1;
constexpr connection_size_t null_connection = -1;

class Circuit {
    struct ElementData;
    struct ConnectionData;

   public:
    template <bool Const>
    class ElementTemplate;
    template <bool Const>
    class InputTemplate;
    template <bool Const>
    class OutputTemplate;

    using Element = ElementTemplate<false>;
    using ConstElement = ElementTemplate<true>;
    using Input = InputTemplate<false>;
    using ConstInput = InputTemplate<true>;
    using Output = OutputTemplate<false>;
    using ConstOutput = OutputTemplate<true>;

    [[nodiscard]] element_id_t element_count() const noexcept;
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] bool is_element_id_valid(element_id_t element_id) const noexcept;
    [[nodiscard]] connection_id_t total_input_count() const noexcept;
    [[nodiscard]] connection_id_t total_output_count() const noexcept;

    [[nodiscard]] Element element(element_id_t element_id);
    [[nodiscard]] ConstElement element(element_id_t element_id) const;
    [[nodiscard]] inline auto elements();
    [[nodiscard]] inline auto elements() const;

    Element add_element(ElementType type, connection_size_t input_count,
                        connection_size_t output_count);
    auto clear() -> void;

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
    element_id_t element_id {null_element};
    connection_size_t index {null_connection};
};

template <bool Const>
class Circuit::ElementTemplate {
    using CircuitType = std::conditional_t<Const, const Circuit, Circuit>;
    using ElementDataType = std::conditional_t<Const, const ElementData, ElementData>;

    friend ElementTemplate<!Const>;
    friend Circuit;
    ElementTemplate(CircuitType &circuit, element_id_t element_id) noexcept;

   public:
    /// This constructor is not regarded as a copy constructor,
    //   so we preserve trivially copyable
    template <bool ConstOther>
    ElementTemplate(ElementTemplate<ConstOther> element) noexcept;

    template <bool ConstOther>
    bool operator==(ElementTemplate<ConstOther> other) const noexcept;

    [[nodiscard]] std::string format() const;

    [[nodiscard]] CircuitType *circuit() const noexcept;
    [[nodiscard]] element_id_t element_id() const noexcept;

    [[nodiscard]] ElementType element_type() const;
    [[nodiscard]] connection_size_t input_count() const;
    [[nodiscard]] connection_size_t output_count() const;
    [[nodiscard]] connection_id_t first_input_id() const;
    [[nodiscard]] connection_id_t input_id(connection_size_t input_index) const;
    [[nodiscard]] connection_id_t first_output_id() const;
    [[nodiscard]] connection_id_t output_id(connection_size_t output_index) const;

    [[nodiscard]] InputTemplate<Const> input(connection_size_t input) const;
    [[nodiscard]] OutputTemplate<Const> output(connection_size_t output) const;

    [[nodiscard]] auto inputs() const;
    [[nodiscard]] auto outputs() const;

   private:
    [[nodiscard]] ElementDataType &element_data_() const;

    CircuitType *circuit_;  // never to be null
    element_id_t element_id_;
};

template <bool Const>
class Circuit::InputTemplate {
    using CircuitType = std::conditional_t<Const, const Circuit, Circuit>;
    using ConnectionDataType
        = std::conditional_t<Const, const ConnectionData, ConnectionData>;

    /// This constructor is not regarded as a copy constructor,
    //   so we preserve trivially copyable
    friend InputTemplate<!Const>;
    friend ElementTemplate<Const>;
    explicit InputTemplate(CircuitType &circuit, element_id_t element_id,
                           connection_size_t input_index,
                           connection_id_t input_id) noexcept;

   public:
    template <bool ConstOther>
    InputTemplate(InputTemplate<ConstOther> input) noexcept;

    template <bool ConstOther>
    bool operator==(InputTemplate<ConstOther> other) const noexcept;

    [[nodiscard]] std::string format() const;

    [[nodiscard]] CircuitType *circuit() const noexcept;
    [[nodiscard]] element_id_t element_id() const noexcept;
    [[nodiscard]] connection_size_t input_index() const noexcept;
    [[nodiscard]] connection_id_t input_id() const noexcept;

    [[nodiscard]] ElementTemplate<Const> element() const;
    [[nodiscard]] bool has_connected_element() const;
    [[nodiscard]] element_id_t connected_element_id() const;
    [[nodiscard]] connection_size_t connected_output_index() const;

    /// @throws if connection doesn't exists. Call has_connected_element to check for
    /// this.
    [[nodiscard]] ElementTemplate<Const> connected_element() const;
    /// @throws if connection doesn't exists. Call has_connected_element to check for
    /// this.
    [[nodiscard]] OutputTemplate<Const> connected_output() const;

    void clear_connection() const;
    template <bool ConstOther>
    void connect(OutputTemplate<ConstOther> output) const;

   private:
    [[nodiscard]] ConnectionDataType &connection_data_() const;

    CircuitType *circuit_;  // never to be null
    element_id_t element_id_;
    connection_size_t input_index_;
    connection_id_t input_id_;
};

template <bool Const>
class Circuit::OutputTemplate {
   private:
    using CircuitType = std::conditional_t<Const, const Circuit, Circuit>;
    using ConnectionDataType
        = std::conditional_t<Const, const ConnectionData, ConnectionData>;

    friend OutputTemplate<!Const>;
    friend ElementTemplate<Const>;
    explicit OutputTemplate(CircuitType &circuit, element_id_t element_id,
                            connection_size_t output_index,
                            connection_id_t output_id) noexcept;

   public:
    template <bool ConstOther>
    OutputTemplate(OutputTemplate<ConstOther> output) noexcept;

    template <bool ConstOther>
    bool operator==(OutputTemplate<ConstOther> other) const noexcept;

    [[nodiscard]] std::string format() const;

    [[nodiscard]] CircuitType *circuit() const noexcept;
    [[nodiscard]] element_id_t element_id() const noexcept;
    [[nodiscard]] connection_size_t output_index() const noexcept;
    [[nodiscard]] connection_id_t output_id() const noexcept;

    [[nodiscard]] ElementTemplate<Const> element() const;
    [[nodiscard]] bool has_connected_element() const;
    [[nodiscard]] element_id_t connected_element_id() const;
    [[nodiscard]] connection_size_t connected_input_index() const;

    /// @throws if connection doesn't exists. Call has_connected_element to check for
    /// this.
    [[nodiscard]] ElementTemplate<Const> connected_element() const;
    /// @throws if connection doesn't exists. Call has_connected_element to check for
    /// this.
    [[nodiscard]] InputTemplate<Const> connected_input() const;

    void clear_connection() const;
    template <bool ConstOther>
    void connect(InputTemplate<ConstOther> input) const;

   private:
    [[nodiscard]] ConnectionDataType &connection_data_() const;

    CircuitType *circuit_;  // never to be null
    element_id_t element_id_;
    connection_size_t output_index_;
    connection_id_t output_id_;
};

void add_output_placeholders(Circuit &circuit);

Circuit benchmark_circuit(int n_elements = 100);

Circuit create_random_circuit(std::mt19937 &rng, int n_elements = 100,
                              double connection_ratio = 0.75);

//
// Circuit
//

// auto return methods need to be defined in the header, so the type can be deduced

auto Circuit::elements() {
    return ranges::views::iota(0, element_count())
           | ranges::views::transform(
               [this](int i) { return this->element(static_cast<element_id_t>(i)); });
}

auto Circuit::elements() const {
    return ranges::views::iota(0, element_count())
           | ranges::views::transform(
               [this](int i) { return this->element(static_cast<element_id_t>(i)); });
}

//
// Circuit::Element
//

template <bool Const>
Circuit::ElementTemplate<Const>::ElementTemplate(CircuitType &circuit,
                                                 element_id_t element_id) noexcept
    : circuit_(&circuit), element_id_(element_id) {}

template <bool Const>
template <bool ConstOther>
Circuit::ElementTemplate<Const>::ElementTemplate(
    ElementTemplate<ConstOther> element) noexcept
    : circuit_(element.circuit_), element_id_(element.element_id_) {
    static_assert(Const || !ConstOther, "Cannot convert ConstElement to Element.");
}

template <bool Const>
template <bool ConstOther>
bool Circuit::ElementTemplate<Const>::operator==(
    ElementTemplate<ConstOther> other) const noexcept {
    return circuit_ == other.circuit_ && element_id_ == other.element_id_;
}

template <bool Const>
std::string Circuit::ElementTemplate<Const>::format() const {
    return fmt::format("<Element {}: {} {} x {}>", element_id(), element_type(),
                       input_count(), output_count());
}

template <bool Const>
auto Circuit::ElementTemplate<Const>::circuit() const noexcept -> CircuitType * {
    return circuit_;
}

template <bool Const>
element_id_t Circuit::ElementTemplate<Const>::element_id() const noexcept {
    return element_id_;
}

template <bool Const>
ElementType Circuit::ElementTemplate<Const>::element_type() const {
    return element_data_().type;
}

template <bool Const>
connection_size_t Circuit::ElementTemplate<Const>::input_count() const {
    return element_data_().input_count;
}

template <bool Const>
connection_size_t Circuit::ElementTemplate<Const>::output_count() const {
    return element_data_().output_count;
}

template <bool Const>
connection_id_t Circuit::ElementTemplate<Const>::first_input_id() const {
    return element_data_().first_input_id;
}

template <bool Const>
connection_id_t Circuit::ElementTemplate<Const>::input_id(
    connection_size_t input_index) const {
    if (input_index < 0 || input_index >= input_count()) [[unlikely]] {
        throw_exception("Index is invalid");
    }
    return first_input_id() + input_index;
}

template <bool Const>
connection_id_t Circuit::ElementTemplate<Const>::first_output_id() const {
    return element_data_().first_output_id;
}

template <bool Const>
connection_id_t Circuit::ElementTemplate<Const>::output_id(
    connection_size_t output_index) const {
    if (output_index < 0 || output_index >= output_count()) [[unlikely]] {
        throw_exception("Index is invalid");
    }
    return first_output_id() + output_index;
}

template <bool Const>
auto Circuit::ElementTemplate<Const>::input(connection_size_t input) const
    -> InputTemplate<Const> {
    return InputTemplate<Const> {*circuit_, element_id_, input, input_id(input)};
}

template <bool Const>
auto Circuit::ElementTemplate<Const>::output(connection_size_t output) const
    -> OutputTemplate<Const> {
    return OutputTemplate<Const> {*circuit_, element_id_, output, output_id(output)};
}

template <bool Const>
inline auto Circuit::ElementTemplate<Const>::inputs() const {
    // We capture the element by value. This is because elements are temporary objects
    // and might be discarded before the lambda is evaluated. Also its very cheap.
    return ranges::views::iota(0, input_count())
           | ranges::views::transform(
               [*this](int i) { return this->input(static_cast<connection_size_t>(i)); });
}

template <bool Const>
inline auto Circuit::ElementTemplate<Const>::outputs() const {
    return ranges::views::iota(0, output_count())
           | ranges::views::transform([*this](int i) {
                 return this->output(static_cast<connection_size_t>(i));
             });
}

template <bool Const>
auto Circuit::ElementTemplate<Const>::element_data_() const -> ElementDataType & {
    return circuit_->element_data_store_.at(element_id_);
}

//
// Circuit::Input
//

template <bool Const>
Circuit::InputTemplate<Const>::InputTemplate(CircuitType &circuit,
                                             element_id_t element_id,
                                             connection_size_t input_index,
                                             connection_id_t input_id) noexcept
    : circuit_(&circuit),
      element_id_(element_id),
      input_index_(input_index),
      input_id_(input_id) {}

template <bool Const>
template <bool ConstOther>
Circuit::InputTemplate<Const>::InputTemplate(InputTemplate<ConstOther> input) noexcept
    : circuit_(input.circuit_),
      element_id_(input.element_id_),
      input_index_(input.input_index_),
      input_id_(input.input_id_) {
    static_assert(Const || !ConstOther, "Cannot convert ConstInput to Input.");
}

template <bool Const>
template <bool ConstOther>
bool Circuit::InputTemplate<Const>::operator==(
    InputTemplate<ConstOther> other) const noexcept {
    return circuit_ == other.circuit_ && element_id_ == other.element_id_
           && input_index_ == other.input_index_ && input_index_ == other.input_index_;
}

template <bool Const>
std::string Circuit::InputTemplate<Const>::format() const {
    const auto element = this->element();
    return fmt::format("<Input {} of Element {}: {} {} x {}>", input_index(),
                       element_id(), element.element_type(), element.input_count(),
                       element.output_count());
}

template <bool Const>
auto Circuit::InputTemplate<Const>::circuit() const noexcept -> CircuitType * {
    return circuit_;
}

template <bool Const>
element_id_t Circuit::InputTemplate<Const>::element_id() const noexcept {
    return element_id_;
}

template <bool Const>
connection_size_t Circuit::InputTemplate<Const>::input_index() const noexcept {
    return input_index_;
}

template <bool Const>
connection_id_t Circuit::InputTemplate<Const>::input_id() const noexcept {
    return input_id_;
}

template <bool Const>
auto Circuit::InputTemplate<Const>::element() const -> ElementTemplate<Const> {
    return circuit_->element(element_id_);
}

template <bool Const>
bool Circuit::InputTemplate<Const>::has_connected_element() const {
    return connected_element_id() != null_element;
}

template <bool Const>
element_id_t Circuit::InputTemplate<Const>::connected_element_id() const {
    return connection_data_().element_id;
}

template <bool Const>
connection_size_t Circuit::InputTemplate<Const>::connected_output_index() const {
    return connection_data_().index;
}

template <bool Const>
auto Circuit::InputTemplate<Const>::connected_element() const -> ElementTemplate<Const> {
    return circuit_->element(connected_element_id());
}

template <bool Const>
auto Circuit::InputTemplate<Const>::connected_output() const -> OutputTemplate<Const> {
    return connected_element().output(connected_output_index());
}

template <bool Const>
void Circuit::InputTemplate<Const>::clear_connection() const {
    static_assert(!Const, "Cannot clear connection for const circuit.");

    auto &connection_data {connection_data_()};
    if (connection_data.element_id != null_element) {
        auto &destination_connection_data {
            circuit_->output_data_store_.at(circuit_->element(connection_data.element_id)
                                                .output_id(connection_data.index))};

        destination_connection_data.element_id = null_element;
        destination_connection_data.index = null_connection;

        connection_data.element_id = null_element;
        connection_data.index = null_connection;
    }
}

template <bool Const>
template <bool ConstOther>
void Circuit::InputTemplate<Const>::connect(OutputTemplate<ConstOther> output) const {
    static_assert(!Const, "Cannot connect input for const circuit.");
    clear_connection();

    // get data before we modify anything, for exception safety
    auto &destination_connection_data {
        circuit_->output_data_store_.at(output.output_id())};
    auto &connection_data {connection_data_()};

    connection_data.element_id = output.element_id();
    connection_data.index = output.output_index();

    destination_connection_data.element_id = element_id();
    destination_connection_data.index = input_index();
}

template <bool Const>
auto Circuit::InputTemplate<Const>::connection_data_() const -> ConnectionDataType & {
    return circuit_->input_data_store_.at(input_id_);
}

//
// Circuit::Output
//

template <bool Const>
Circuit::OutputTemplate<Const>::OutputTemplate(CircuitType &circuit,
                                               element_id_t element_id,
                                               connection_size_t output_index,
                                               connection_id_t output_id) noexcept
    : circuit_(&circuit),
      element_id_(element_id),
      output_index_(output_index),
      output_id_(output_id) {}

template <bool Const>
template <bool ConstOther>
Circuit::OutputTemplate<Const>::OutputTemplate(OutputTemplate<ConstOther> output) noexcept
    : circuit_(output.circuit_),
      element_id_(output.element_id_),
      output_index_(output.output_index_),
      output_id_(output.output_id_) {
    static_assert(Const || !ConstOther, "Cannot convert ConstOutput to Output.");
}

template <bool Const>
template <bool ConstOther>
bool Circuit::OutputTemplate<Const>::operator==(
    Circuit::OutputTemplate<ConstOther> other) const noexcept {
    return circuit_ == other.circuit_ && element_id_ == other.element_id_
           && output_index_ == other.output_index_ && output_id_ == other.output_id_;
}

template <bool Const>
std::string Circuit::OutputTemplate<Const>::format() const {
    const auto element = this->element();
    return fmt::format("<Output {} of Element {}: {} {} x {}>", output_index(),
                       element_id(), element.element_type(), element.input_count(),
                       element.output_count());
}

template <bool Const>
auto Circuit::OutputTemplate<Const>::circuit() const noexcept -> CircuitType * {
    return circuit_;
}

template <bool Const>
element_id_t Circuit::OutputTemplate<Const>::element_id() const noexcept {
    return element_id_;
}

template <bool Const>
connection_size_t Circuit::OutputTemplate<Const>::output_index() const noexcept {
    return output_index_;
}

template <bool Const>
connection_id_t Circuit::OutputTemplate<Const>::output_id() const noexcept {
    return output_id_;
}

template <bool Const>
auto Circuit::OutputTemplate<Const>::element() const -> ElementTemplate<Const> {
    return circuit_->element(element_id_);
}

template <bool Const>
bool Circuit::OutputTemplate<Const>::has_connected_element() const {
    return connected_element_id() != null_element;
}

template <bool Const>
element_id_t Circuit::OutputTemplate<Const>::connected_element_id() const {
    return connection_data_().element_id;
}

template <bool Const>
connection_size_t Circuit::OutputTemplate<Const>::connected_input_index() const {
    return connection_data_().index;
}

template <bool Const>
auto Circuit::OutputTemplate<Const>::connected_element() const -> ElementTemplate<Const> {
    return circuit_->element(connected_element_id());
}

template <bool Const>
auto Circuit::OutputTemplate<Const>::connected_input() const -> InputTemplate<Const> {
    return connected_element().input(connected_input_index());
}

template <bool Const>
void Circuit::OutputTemplate<Const>::clear_connection() const {
    static_assert(!Const, "Cannot clear connection for const circuit.");

    auto &connection_data {connection_data_()};
    if (connection_data.element_id != null_element) {
        auto &destination_connection_data
            = circuit_->input_data_store_.at(circuit_->element(connection_data.element_id)
                                                 .input_id(connection_data.index));

        destination_connection_data.element_id = null_element;
        destination_connection_data.index = null_connection;

        connection_data.element_id = null_element;
        connection_data.index = null_connection;
    }
}

template <bool Const>
template <bool ConstOther>
void Circuit::OutputTemplate<Const>::connect(InputTemplate<ConstOther> input) const {
    static_assert(!Const, "Cannot connect output for const circuit.");
    clear_connection();

    // get data before we modify anything, for exception safety
    auto &connection_data {connection_data_()};
    auto &destination_connection_data {circuit_->input_data_store_.at(input.input_id())};

    connection_data.element_id = input.element_id();
    connection_data.index = input.input_index();

    destination_connection_data.element_id = element_id();
    destination_connection_data.index = output_index();
}

template <bool Const>
auto Circuit::OutputTemplate<Const>::connection_data_() const -> ConnectionDataType & {
    return circuit_->output_data_store_.at(output_id_);
}

}  // namespace logicsim

template <>
struct fmt::formatter<logicsim::ElementType> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const logicsim::ElementType &obj, fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", ::logicsim::format(obj));
    }
};

template <>
struct fmt::formatter<logicsim::Circuit::Element> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const logicsim::Circuit::Element &obj, fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

template <>
struct fmt::formatter<logicsim::Circuit::ConstElement> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const logicsim::Circuit::Element &obj, fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

template <>
struct fmt::formatter<logicsim::Circuit::Input> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const logicsim::Circuit::Input &obj, fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

template <>
struct fmt::formatter<logicsim::Circuit::ConstInput> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const logicsim::Circuit::ConstInput &obj,
                fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

template <>
struct fmt::formatter<logicsim::Circuit::Output> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const logicsim::Circuit::Output &obj, fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

template <>
struct fmt::formatter<logicsim::Circuit::ConstOutput> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const logicsim::Circuit::ConstOutput &obj,
                fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

#endif