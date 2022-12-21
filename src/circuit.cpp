
#include "circuit.h"

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <range/v3/all.hpp>

#include <utility>

// generator<std::tuple<int, int, int>> pytriples() {
//     for (int z = 1;; ++z)
//         for (int x = 1; x <= z; ++x)
//             for (int y = x; y <= z; ++y)
//                 if (x * x + y * y == z * z) co_yield std::make_tuple(x, y, z);
// }

namespace logicsim {

auto format(ElementType type) -> std::string {
    switch (type) {
        case ElementType::placeholder:
            return "Placeholder";
        case ElementType::wire:
            return "Wire";
        case ElementType::inverter_element:
            return "Inverter";
        case ElementType::and_element:
            return "AndElement";
        case ElementType::or_element:
            return "OrElement";
        case ElementType::xor_element:
            return "XorElement";
    }
    throw_exception("Don't know how to convert ElementType to string.");
}

//
// Circuit
//

auto Circuit::format() const -> std::string {
    std::string inner;
    if (!empty()) {
        auto element_strings = ranges::views::transform(
            elements(), [&](auto element) { return element.format(true); });
        inner = fmt::format(": [\n  {}\n]", fmt::join(element_strings, ",\n  "));
    }
    return fmt::format("<Circuit with {} elements{}>", element_count(), inner);
}

auto Circuit::element_count() const noexcept -> element_id_t {
    return static_cast<element_id_t>(element_data_store_.size());
}

auto Circuit::empty() const noexcept -> bool { return element_data_store_.empty(); }

auto Circuit::is_element_id_valid(element_id_t element_id) const noexcept -> bool {
    return element_id >= 0 && element_id < element_count();
}

auto Circuit::element(element_id_t element_id) -> Element {
    if (!is_element_id_valid(element_id)) [[unlikely]] {
        throw_exception("Element id is invalid");
    }
    return Element {*this, element_id};
}

auto Circuit::element(element_id_t element_id) const -> ConstElement {
    if (!is_element_id_valid(element_id)) [[unlikely]] {
        throw_exception("Element id is invalid");
    }
    return ConstElement {*this, element_id};
}

auto Circuit::elements() -> ranges::any_view<Element, ranges::category::random_access
                                                          | ranges::category::sized> {
    return ranges::views::iota(element_id_t {0}, element_count())
           | ranges::views::transform(
               [this](element_id_t element_id) { return this->element(element_id); });
}

auto Circuit::elements() const
    -> ranges::any_view<ConstElement,
                        ranges::category::random_access | ranges::category::sized> {
    return ranges::views::iota(element_id_t {0}, element_count())
           | ranges::views::transform(
               [this](element_id_t element_id) { return this->element(element_id); });
}

auto Circuit::add_element(ElementType type, connection_size_t input_count,
                          connection_size_t output_count) -> Element {
    if (input_count < 0) [[unlikely]] {
        throw_exception("Input count needs to be positive.");
    }
    if (output_count < 0) [[unlikely]] {
        throw_exception("Output count needs to be positive.");
    }

    const auto new_input_size {input_data_store_.size() + input_count};
    const auto new_output_size {output_data_store_.size() + output_count};

    // make sure we can represent all ids
    if (element_data_store_.size() + 1 >= std::numeric_limits<element_id_t>::max())
        [[unlikely]] {
        throw_exception("Reached maximum number of elements.");
    }
    if (new_input_size >= std::numeric_limits<connection_id_t>::max()) [[unlikely]] {
        throw_exception("Reached maximum number of inputs.");
    }
    if (new_output_size >= std::numeric_limits<connection_id_t>::max()) [[unlikely]] {
        throw_exception("Reached maximum number of outputs.");
    }
    // TODO create custom exception, as we want to handle theses ones.

    element_data_store_.push_back(
        {static_cast<connection_id_t>(input_data_store_.size()),
         static_cast<connection_id_t>(output_data_store_.size()), input_count,
         output_count, type});
    input_data_store_.resize(new_input_size);
    output_data_store_.resize(new_output_size);

    element_id_t element_id {static_cast<element_id_t>(element_data_store_.size() - 1)};
    return element(element_id);
}

auto Circuit::clear() -> void {
    element_data_store_.clear();
    output_data_store_.clear();
    input_data_store_.clear();
}

auto Circuit::total_input_count() const noexcept -> connection_id_t {
    return static_cast<connection_id_t>(input_data_store_.size());
}

auto Circuit::total_output_count() const noexcept -> connection_id_t {
    return static_cast<connection_id_t>(output_data_store_.size());
}

auto validate_output_connected(const Circuit::ConstOutput output) -> void {
    if (!output.has_connected_element()) [[unlikely]] {
        throw_exception("Element has unconnected output.");
    }
}

auto validate_outputs_connected(const Circuit::ConstElement element) -> void {
    ranges::for_each(element.outputs(), validate_output_connected);
}

auto validate_input_consistent(const Circuit::ConstInput input) -> void {
    if (input.has_connected_element()) {
        auto back_reference {input.connected_output().connected_input()};
        if (back_reference != input) [[unlikely]] {
            throw_exception("Back reference doesn't match.");
        }
    }
}

auto validate_output_consistent(const Circuit::ConstOutput output) -> void {
    if (output.has_connected_element()) {
        if (!output.connected_input().has_connected_element()) [[unlikely]] {
            throw_exception("Back reference is missing.");
        }
        auto back_reference {output.connected_input().connected_output()};
        if (back_reference != output) [[unlikely]] {
            throw_exception("Back reference doesn't match.");
        }
    }
}

auto validate_element_connections_consistent(const Circuit::ConstElement element)
    -> void {
    ranges::for_each(element.inputs(), validate_input_consistent);
    ranges::for_each(element.outputs(), validate_output_consistent);
}

auto Circuit::validate_connection_data_(const Circuit::ConnectionData connection_data)
    -> void {
    if (connection_data.element_id != null_element
        && connection_data.index == null_connection) [[unlikely]] {
        throw_exception("Connection to an element cannot have null_connection.");
    }

    if (connection_data.element_id == null_element
        && connection_data.index != null_connection) [[unlikely]] {
        throw_exception("Connection with null_element requires null_connection.");
    }
}

auto Circuit::validate(bool require_all_outputs_connected) const -> void {
    auto all_one {[](auto vector) {
        return ranges::all_of(vector, [](auto item) { return item == 1; });
    }};

    //  every output_data entry is referenced once
    std::vector<int> input_reference_count(total_input_count(), 0);
    for (auto element : elements()) {
        for (auto input : element.inputs()) {
            input_reference_count.at(input.input_id()) += 1;
        }
    }
    if (!all_one(input_reference_count)) [[unlikely]] {
        throw_exception("Input data is inconsistent");
    }

    //  every output_data entry is referenced once
    std::vector<int> output_reference_count(total_output_count(), 0);
    for (auto element : elements()) {
        for (auto output : element.outputs()) {
            output_reference_count.at(output.output_id()) += 1;
        }
    }
    if (!all_one(output_reference_count)) [[unlikely]] {
        throw_exception("Output data is inconsistent");
    }

    // connection data valid
    ranges::for_each(input_data_store_, Circuit::validate_connection_data_);
    ranges::for_each(output_data_store_, Circuit::validate_connection_data_);

    // back references consistent
    ranges::for_each(elements(), validate_element_connections_consistent);

    // all outputs connected
    if (require_all_outputs_connected) {
        ranges::for_each(elements(), validate_outputs_connected);
    }
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
std::string Circuit::ElementTemplate<Const>::format(bool with_connections) const {
    auto connections = !with_connections ? ""
                                         : fmt::format(", inputs = {}, outputs = {}",
                                                       format_inputs(), format_outputs());

    return fmt::format("<Element {}: {}x{} {}{}>", element_id(), input_count(),
                       output_count(), element_type(), connections);
}

template <bool Const>
std::string Circuit::ElementTemplate<Const>::format_inputs() const {
    auto strings = ranges::views::transform(
        inputs(), [](auto input) { return input.format_connection(); });
    return fmt::format("{::s}", strings);
}

template <bool Const>
std::string Circuit::ElementTemplate<Const>::format_outputs() const {
    auto strings = ranges::views::transform(
        outputs(), [](auto output) { return output.format_connection(); });
    return fmt::format("{::s}", strings);
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
inline auto Circuit::ElementTemplate<Const>::inputs() const
    -> ranges::any_view<InputTemplate<Const>,
                        ranges::category::random_access | ranges::category::sized> {
    // We capture the element by value. This is because elements are temporary objects
    // and might be discarded before the lambda is evaluated. Also it's very cheap.
    return ranges::views::iota(connection_size_t {0}, input_count())
           | ranges::views::transform(
               [*this](connection_size_t input_id) { return this->input(input_id); });
}

template <bool Const>
inline auto Circuit::ElementTemplate<Const>::outputs() const
    -> ranges::any_view<OutputTemplate<Const>,
                        ranges::category::random_access | ranges::category::sized> {
    return ranges::views::iota(connection_size_t {0}, output_count())
           | ranges::views::transform(
               [*this](int output_id) { return this->output(output_id); });
}

template <bool Const>
auto Circuit::ElementTemplate<Const>::element_data_() const -> ElementDataType & {
    return circuit_->element_data_store_.at(element_id_);
}

// Template Instanciations

template class Circuit::ElementTemplate<true>;
template class Circuit::ElementTemplate<false>;

template Circuit::ElementTemplate<true>::ElementTemplate(ElementTemplate<false>) noexcept;

template bool Circuit::ElementTemplate<false>::operator==<false>(
    ElementTemplate<false>) const noexcept;
template bool Circuit::ElementTemplate<false>::operator==<true>(
    ElementTemplate<true>) const noexcept;
template bool Circuit::ElementTemplate<true>::operator==<false>(
    ElementTemplate<false>) const noexcept;
template bool Circuit::ElementTemplate<true>::operator==<true>(
    ElementTemplate<true>) const noexcept;

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
std::string Circuit::InputTemplate<Const>::format_connection() const {
    if (has_connected_element()) {
        return fmt::format("Element_{}-{}", connected_element_id(),
                           connected_output_index());
    }
    return "---";
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

template <>
void Circuit::InputTemplate<false>::clear_connection() const {
    // static_assert(!Const, "Cannot clear connection for const circuit.");

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

// Template Instanciations

template class Circuit::InputTemplate<true>;
template class Circuit::InputTemplate<false>;

template Circuit::InputTemplate<true>::InputTemplate(InputTemplate<false>) noexcept;

template bool Circuit::InputTemplate<false>::operator==<false>(
    InputTemplate<false>) const noexcept;
template bool Circuit::InputTemplate<false>::operator==<true>(
    InputTemplate<true>) const noexcept;
template bool Circuit::InputTemplate<true>::operator==<false>(
    InputTemplate<false>) const noexcept;
template bool Circuit::InputTemplate<true>::operator==<true>(
    InputTemplate<true>) const noexcept;

template void Circuit::InputTemplate<false>::connect<false>(OutputTemplate<false>) const;
template void Circuit::InputTemplate<false>::connect<true>(OutputTemplate<true>) const;

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
std::string Circuit::OutputTemplate<Const>::format_connection() const {
    if (has_connected_element()) {
        return fmt::format("Element_{}-{}", connected_element_id(),
                           connected_input_index());
    }
    return "---";
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

template <>
void Circuit::OutputTemplate<false>::clear_connection() const {
    // static_assert(!Const, "Cannot clear connection for const circuit.");

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

// Template Instanciations

template class Circuit::OutputTemplate<true>;
template class Circuit::OutputTemplate<false>;

template Circuit::OutputTemplate<true>::OutputTemplate(OutputTemplate<false>) noexcept;

template bool Circuit::OutputTemplate<false>::operator==<false>(
    OutputTemplate<false>) const noexcept;
template bool Circuit::OutputTemplate<false>::operator==<true>(
    OutputTemplate<true>) const noexcept;
template bool Circuit::OutputTemplate<true>::operator==<false>(
    OutputTemplate<false>) const noexcept;
template bool Circuit::OutputTemplate<true>::operator==<true>(
    OutputTemplate<true>) const noexcept;

template void Circuit::OutputTemplate<false>::connect<false>(InputTemplate<false>) const;
template void Circuit::OutputTemplate<false>::connect<true>(InputTemplate<true>) const;

//
// Free Functions
//

auto add_placeholder(Circuit::Output output) -> void {
    if (!output.has_connected_element()) {
        auto placeholder {output.circuit()->add_element(ElementType::placeholder, 1, 0)};
        output.connect(placeholder.input(0));
    }
}

auto add_element_placeholders(Circuit::Element element) -> void {
    ranges::for_each(element.outputs(), add_placeholder);
}

auto add_output_placeholders(Circuit &circuit) -> void {
    ranges::for_each(circuit.elements(), add_element_placeholders);
}

auto benchmark_circuit(const int n_elements) -> Circuit {
    Circuit circuit {};

    auto elem0 {circuit.add_element(ElementType::and_element, 2, 2)};

    for ([[maybe_unused]] auto count : ranges::iota_view(0, n_elements - 1)) {
        auto wire0 {circuit.add_element(ElementType::wire, 1, 1)};
        auto wire1 {circuit.add_element(ElementType::wire, 1, 1)};
        auto elem1 {circuit.add_element(ElementType::and_element, 2, 2)};

        elem0.output(0).connect(wire0.input(0));
        elem0.output(1).connect(wire1.input(0));

        wire0.output(0).connect(elem1.input(0));
        wire1.output(0).connect(elem1.input(1));

        elem0 = elem1;
    }

    return circuit;
}

}  // namespace logicsim

//
// Formatters
//
