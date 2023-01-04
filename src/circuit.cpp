
#include "circuit.h"

#include "algorithm.h"
#include "exceptions.h"
#include "format.h"
#include "iterator.h"
#include "random.h"
#include "range.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <fmt/core.h>
#include <gsl/gsl>

#include <algorithm>
#include <utility>

namespace logicsim {

auto format(ElementType type) -> std::string {
    switch (type) {
        using enum ElementType;

        case placeholder:
            return "Placeholder";
        case wire:
            return "Wire";
        case inverter_element:
            return "Inverter";
        case and_element:
            return "AndElement";
        case or_element:
            return "OrElement";
        case xor_element:
            return "XorElement";
        case clock_element:
            return "ClockElement";
        case flipflop_jk:
            return "JK-FlipFlop";
    }
    throw_exception("Don't know how to convert ElementType to string.");
}

//
// Circuit
//

auto Circuit::format() const -> std::string {
    std::string inner;
    if (!empty()) {
        auto format_true = [](auto element) { return element.format(true); };
        inner = fmt::format(": [\n  {}\n]", fmt_join(elements(), ",\n  ", format_true));
    }
    return fmt::format("<Circuit with {} elements{}>", element_count(), inner);
}

auto Circuit::element_count() const noexcept -> element_id_t {
    return static_cast<element_id_t>(element_data_store_.size());
}

auto Circuit::empty() const noexcept -> bool {
    return element_data_store_.empty();
}

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

auto Circuit::elements() noexcept -> ElementView {
    return ElementView {*this};
}

auto Circuit::elements() const noexcept -> ConstElementView {
    return ConstElementView {*this};
}

auto Circuit::add_element(ElementType type, connection_size_t input_count,
                          connection_size_t output_count) -> Element {
    if (input_count < 0) [[unlikely]] {
        throw_exception("Input count needs to be positive.");
    }
    if (output_count < 0) [[unlikely]] {
        throw_exception("Output count needs to be positive.");
    }

    // TODO handle overflow when adding to many connections

    // make sure we can represent all ids
    if (element_data_store_.size() + 1 >= std::numeric_limits<element_id_t>::max())
        [[unlikely]] {
        throw_exception("Reached maximum number of elements.");
    }

    element_data_store_.push_back({.input_count = input_count,
                                   .output_count = output_count,
                                   .type = type,
                                   .input_data = {},
                                   .output_data = {}});
    element_id_t element_id {static_cast<element_id_t>(element_data_store_.size() - 1)};

    element_data_store_.at(element_id).input_data.resize(input_count);
    element_data_store_.at(element_id).output_data.resize(output_count);

    input_count_ += input_count;
    output_count_ += output_count;

    return element(element_id);
}

auto Circuit::clear() -> void {
    element_data_store_.clear();
    input_count_ = 0;
    output_count_ = 0;
}

auto Circuit::input_count() const noexcept -> connection_id_t {
    return input_count_;
}

auto Circuit::output_count() const noexcept -> connection_id_t {
    return output_count_;
}

auto validate_output_connected(const Circuit::ConstOutput output) -> void {
    if (!output.has_connected_element()) [[unlikely]] {
        throw_exception("Element has unconnected output.");
    }
}

auto validate_outputs_connected(const Circuit::ConstElement element) -> void {
    std::ranges::for_each(element.outputs(), validate_output_connected);
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
    std::ranges::for_each(element.inputs(), validate_input_consistent);
    std::ranges::for_each(element.outputs(), validate_output_consistent);
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
    // TODO do we need this still?
    // TODO What else shall we check?

    //  every output_data entry is referenced once
    // std::vector<int> input_reference_count(input_count(), 0);
    // for (auto element : elements()) {
    //    for (auto input : element.inputs()) {
    //        input_reference_count.at(input.input_id()) += 1;
    //    }
    //}
    // if (!all_equal(input_reference_count, 1)) [[unlikely]] {
    //    throw_exception("Input data is inconsistent");
    //}
    //
    //  every output_data entry is referenced once
    // std::vector<int> output_reference_count(output_count(), 0);
    // for (auto element : elements()) {
    //    for (auto output : element.outputs()) {
    //        output_reference_count.at(output.output_id()) += 1;
    //    }
    //}
    // if (!all_equal(output_reference_count, 1)) [[unlikely]] {
    //    throw_exception("Output data is inconsistent");
    //}

    // connection data valid
    // TODO impelement
    // std::ranges::for_each(input_data_store_, Circuit::validate_connection_data_);
    // std::ranges::for_each(output_data_store_, Circuit::validate_connection_data_);

    // back references consistent
    std::ranges::for_each(elements(), validate_element_connections_consistent);

    // all outputs connected
    if (require_all_outputs_connected) {
        std::ranges::for_each(elements(), validate_outputs_connected);
    }
}

//
// Element Iterator
//

template <bool Const>
Circuit::ElementIteratorTemplate<Const>::ElementIteratorTemplate(
    circuit_type &circuit, element_id_t element_id) noexcept
    : circuit_(&circuit), element_id_(element_id) {}

template <bool Const>
auto Circuit::ElementIteratorTemplate<Const>::operator*() const -> value_type {
    if (circuit_ == nullptr) [[unlikely]] {
        throw_exception("circuit cannot be null when dereferencing element iterator");
    }
    return circuit_->element(element_id_);
}

template <bool Const>
auto Circuit::ElementIteratorTemplate<Const>::operator++() noexcept
    -> Circuit::ElementIteratorTemplate<Const> & {
    ++element_id_;
    return *this;
}

template <bool Const>
auto Circuit::ElementIteratorTemplate<Const>::operator++(int) noexcept
    -> Circuit::ElementIteratorTemplate<Const> {
    auto tmp = *this;
    ++element_id_;
    return tmp;
}

template <bool Const>
auto Circuit::ElementIteratorTemplate<Const>::operator==(
    const ElementIteratorTemplate &right) const noexcept -> bool {
    return element_id_ >= right.element_id_;
}

template <bool Const>
auto Circuit::ElementIteratorTemplate<Const>::operator-(
    const ElementIteratorTemplate &right) const noexcept -> difference_type {
    return element_id_ - right.element_id_;
}

template class Circuit::ElementIteratorTemplate<false>;
template class Circuit::ElementIteratorTemplate<true>;

//
// Element View
//

template <bool Const>
Circuit::ElementViewTemplate<Const>::ElementViewTemplate(circuit_type &circuit) noexcept
    : circuit_(&circuit) {}

template <bool Const>
auto Circuit::ElementViewTemplate<Const>::begin() const noexcept -> iterator_type {
    return iterator_type {*circuit_, 0};
}

template <bool Const>
auto Circuit::ElementViewTemplate<Const>::end() const noexcept -> iterator_type {
    return iterator_type {*circuit_, circuit_->element_count()};
}

template <bool Const>
auto Circuit::ElementViewTemplate<Const>::size() const noexcept -> element_id_t {
    return circuit_->element_count();
}

template <bool Const>
auto Circuit::ElementViewTemplate<Const>::empty() const noexcept -> bool {
    return circuit_->empty();
}

template class Circuit::ElementViewTemplate<false>;
template class Circuit::ElementViewTemplate<true>;

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
    requires Const && (!ConstOther)
    : circuit_(element.circuit_), element_id_(element.element_id_) {}

template <bool Const>
template <bool ConstOther>
auto Circuit::ElementTemplate<Const>::operator==(
    ElementTemplate<ConstOther> other) const noexcept -> bool {
    return circuit_ == other.circuit_ && element_id_ == other.element_id_;
}

template <bool Const>
auto Circuit::ElementTemplate<Const>::format(bool with_connections) const -> std::string {
    auto connections = !with_connections
                           ? ""
                           : fmt::format(", inputs = {}, outputs = {}", inputs().format(),
                                         outputs().format());

    return fmt::format("<Element {}: {}x{} {}{}>", element_id(), input_count(),
                       output_count(), element_type(), connections);
}

template <bool Const>
auto Circuit::ElementTemplate<Const>::circuit() const noexcept -> CircuitType * {
    return circuit_;
}

template <bool Const>
auto Circuit::ElementTemplate<Const>::element_id() const noexcept -> element_id_t {
    return element_id_;
}

template <bool Const>
auto Circuit::ElementTemplate<Const>::element_type() const -> ElementType {
    return element_data_().type;
}

template <bool Const>
auto Circuit::ElementTemplate<Const>::input_count() const -> connection_size_t {
    return element_data_().input_count;
}

template <bool Const>
auto Circuit::ElementTemplate<Const>::output_count() const -> connection_size_t {
    return element_data_().output_count;
}

template <bool Const>
auto Circuit::ElementTemplate<Const>::input(connection_size_t input) const
    -> InputTemplate<Const> {
    return InputTemplate<Const> {*circuit_, element_id_, input};
}

template <bool Const>
auto Circuit::ElementTemplate<Const>::output(connection_size_t output) const
    -> OutputTemplate<Const> {
    return OutputTemplate<Const> {*circuit_, element_id_, output};
}

template <bool Const>
inline auto Circuit::ElementTemplate<Const>::inputs() const -> InputViewTemplate<Const> {
    return InputViewTemplate<Const> {*this};
}

template <bool Const>
inline auto Circuit::ElementTemplate<Const>::outputs() const
    -> OutputViewTemplate<Const> {
    return OutputViewTemplate<Const> {*this};
}

template <bool Const>
auto Circuit::ElementTemplate<Const>::element_data_() const -> ElementDataType & {
    return circuit_->element_data_store_.at(element_id_);
}

// Template Instanciations

template class Circuit::ElementTemplate<true>;
template class Circuit::ElementTemplate<false>;

template Circuit::ElementTemplate<true>::ElementTemplate(ElementTemplate<false>) noexcept;

template auto Circuit::ElementTemplate<false>::operator==<false>(
    ElementTemplate<false>) const noexcept -> bool;
template auto Circuit::ElementTemplate<false>::operator==<true>(
    ElementTemplate<true>) const noexcept -> bool;
template auto Circuit::ElementTemplate<true>::operator==<false>(
    ElementTemplate<false>) const noexcept -> bool;
template auto Circuit::ElementTemplate<true>::operator==<true>(
    ElementTemplate<true>) const noexcept -> bool;

//
// Connection Iterator
//

template <bool Const, bool IsInput>
auto Circuit::ConnectionIteratorTemplate<Const, IsInput>::operator*() const
    -> value_type {
    if (!element.has_value()) [[unlikely]] {
        throw_exception("iterator needs a valid element");
    }
    if constexpr (IsInput) {
        return element->input(connection_id);
    } else {
        return element->output(connection_id);
    }
}

template <bool Const, bool IsInput>
auto Circuit::ConnectionIteratorTemplate<Const, IsInput>::operator++() noexcept
    -> Circuit::ConnectionIteratorTemplate<Const, IsInput> & {
    ++connection_id;
    return *this;
}

template <bool Const, bool IsInput>
auto Circuit::ConnectionIteratorTemplate<Const, IsInput>::operator++(int) noexcept
    -> ConnectionIteratorTemplate {
    auto tmp = *this;
    ++connection_id;
    return tmp;
}

template <bool Const, bool IsInput>
auto Circuit::ConnectionIteratorTemplate<Const, IsInput>::operator==(
    const ConnectionIteratorTemplate &right) const noexcept -> bool {
    return connection_id >= right.connection_id;
}

template <bool Const, bool IsInput>
auto Circuit::ConnectionIteratorTemplate<Const, IsInput>::operator-(
    const ConnectionIteratorTemplate &right) const noexcept -> difference_type {
    return connection_id - right.connection_id;
}

template class Circuit::ConnectionIteratorTemplate<false, false>;
template class Circuit::ConnectionIteratorTemplate<true, false>;
template class Circuit::ConnectionIteratorTemplate<false, true>;
template class Circuit::ConnectionIteratorTemplate<true, true>;

//
// Connection View
//

template <bool Const, bool IsInput>
Circuit::ConnectionViewTemplate<Const, IsInput>::ConnectionViewTemplate(
    ElementTemplate<Const> element) noexcept
    : element_(element) {}

template <bool Const, bool IsInput>
auto Circuit::ConnectionViewTemplate<Const, IsInput>::begin() const -> iterator_type {
    return iterator_type {element_, 0};
}

template <bool Const, bool IsInput>
auto Circuit::ConnectionViewTemplate<Const, IsInput>::end() const -> iterator_type {
    return iterator_type {element_, size()};
}

template <bool Const, bool IsInput>
auto Circuit::ConnectionViewTemplate<Const, IsInput>::size() const -> connection_size_t {
    if constexpr (IsInput) {
        return element_.input_count();
    } else {
        return element_.output_count();
    }
}

template <bool Const, bool IsInput>
auto Circuit::ConnectionViewTemplate<Const, IsInput>::empty() const -> bool {
    return size() == 0;
}

template <bool Const, bool IsInput>
auto Circuit::ConnectionViewTemplate<Const, IsInput>::format() const -> std::string {
    auto connections = transform_view(*this, &value_type::format_connection);
    return fmt::format("{}", connections);
}

template class Circuit::ConnectionViewTemplate<false, false>;
template class Circuit::ConnectionViewTemplate<true, false>;
template class Circuit::ConnectionViewTemplate<false, true>;
template class Circuit::ConnectionViewTemplate<true, true>;

//
// Circuit::Input
//

template <bool Const>
Circuit::InputTemplate<Const>::InputTemplate(CircuitType &circuit,
                                             element_id_t element_id,
                                             connection_size_t input_index) noexcept
    : circuit_(&circuit), element_id_(element_id), input_index_(input_index) {}

template <bool Const>
template <bool ConstOther>
Circuit::InputTemplate<Const>::InputTemplate(InputTemplate<ConstOther> input) noexcept
    requires Const && (!ConstOther)
    : circuit_(input.circuit_),
      element_id_(input.element_id_),
      input_index_(input.input_index_) {}

template <bool Const>
template <bool ConstOther>
auto Circuit::InputTemplate<Const>::operator==(
    InputTemplate<ConstOther> other) const noexcept -> bool {
    return circuit_ == other.circuit_ && element_id_ == other.element_id_
           && input_index_ == other.input_index_;
}

template <bool Const>
auto Circuit::InputTemplate<Const>::format() const -> std::string {
    const auto element = this->element();
    return fmt::format("<Input {} of Element {}: {} {} x {}>", input_index(),
                       element_id(), element.element_type(), element.input_count(),
                       element.output_count());
}

template <bool Const>
auto Circuit::InputTemplate<Const>::format_connection() const -> std::string {
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
auto Circuit::InputTemplate<Const>::element_id() const noexcept -> element_id_t {
    return element_id_;
}

template <bool Const>
auto Circuit::InputTemplate<Const>::input_index() const noexcept -> connection_size_t {
    return input_index_;
}

template <bool Const>
auto Circuit::InputTemplate<Const>::element() const -> ElementTemplate<Const> {
    return circuit_->element(element_id_);
}

template <bool Const>
auto Circuit::InputTemplate<Const>::has_connected_element() const -> bool {
    return connected_element_id() != null_element;
}

template <bool Const>
auto Circuit::InputTemplate<Const>::connected_element_id() const -> element_id_t {
    return connection_data_().element_id;
}

template <bool Const>
auto Circuit::InputTemplate<Const>::connected_output_index() const -> connection_size_t {
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
void Circuit::InputTemplate<Const>::clear_connection() const
    requires(!Const)
{
    auto &connection_data {connection_data_()};
    if (connection_data.element_id != null_element) {
        auto &destination_connection_data {
            circuit_->element_data_store_.at(connection_data.element_id)
                .output_data.at(connection_data.index)};

        // circuit_->output_data_store_.at(
        //      circuit_->element(connection_data.element_id)
        //      .output_id(connection_data.index))};

        destination_connection_data.element_id = null_element;
        destination_connection_data.index = null_connection;

        connection_data.element_id = null_element;
        connection_data.index = null_connection;
    }
}

template <bool Const>
template <bool ConstOther>
void Circuit::InputTemplate<Const>::connect(OutputTemplate<ConstOther> output) const
    requires(!Const)
{
    clear_connection();

    // get data before we modify anything, for exception safety
    auto &destination_connection_data
        = circuit_->element_data_store_.at(output.element_id())
              .output_data.at(output.output_index());
    //     circuit_->output_data_store_.at(output.output_id());

    auto &connection_data {connection_data_()};

    connection_data.element_id = output.element_id();
    connection_data.index = output.output_index();

    destination_connection_data.element_id = element_id();
    destination_connection_data.index = input_index();
}

template <bool Const>
auto Circuit::InputTemplate<Const>::connection_data_() const -> ConnectionDataType & {
    // return circuit_->input_data_store_.at(input_id_);
    return circuit_->element_data_store_.at(element_id_).input_data.at(input_index_);
}

// Template Instanciations

template class Circuit::InputTemplate<true>;
template class Circuit::InputTemplate<false>;

template Circuit::InputTemplate<true>::InputTemplate(InputTemplate<false>) noexcept;

template auto Circuit::InputTemplate<false>::operator==<false>(
    InputTemplate<false>) const noexcept -> bool;
template auto Circuit::InputTemplate<false>::operator==<true>(
    InputTemplate<true>) const noexcept -> bool;
template auto Circuit::InputTemplate<true>::operator==<false>(
    InputTemplate<false>) const noexcept -> bool;
template auto Circuit::InputTemplate<true>::operator==<true>(
    InputTemplate<true>) const noexcept -> bool;

template void Circuit::InputTemplate<false>::connect<false>(OutputTemplate<false>) const;
template void Circuit::InputTemplate<false>::connect<true>(OutputTemplate<true>) const;

//
// Circuit::Output
//

template <bool Const>
Circuit::OutputTemplate<Const>::OutputTemplate(CircuitType &circuit,
                                               element_id_t element_id,
                                               connection_size_t output_index) noexcept
    : circuit_(&circuit), element_id_(element_id), output_index_(output_index) {}

template <bool Const>
template <bool ConstOther>
Circuit::OutputTemplate<Const>::OutputTemplate(OutputTemplate<ConstOther> output) noexcept
    requires Const && (!ConstOther)
    : circuit_(output.circuit_),
      element_id_(output.element_id_),
      output_index_(output.output_index_) {}

template <bool Const>
template <bool ConstOther>
auto Circuit::OutputTemplate<Const>::operator==(
    Circuit::OutputTemplate<ConstOther> other) const noexcept -> bool {
    return circuit_ == other.circuit_ && element_id_ == other.element_id_
           && output_index_ == other.output_index_;
}

template <bool Const>
auto Circuit::OutputTemplate<Const>::format() const -> std::string {
    const auto element = this->element();
    return fmt::format("<Output {} of Element {}: {} {} x {}>", output_index(),
                       element_id(), element.element_type(), element.input_count(),
                       element.output_count());
}

template <bool Const>
auto Circuit::OutputTemplate<Const>::format_connection() const -> std::string {
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
auto Circuit::OutputTemplate<Const>::element_id() const noexcept -> element_id_t {
    return element_id_;
}

template <bool Const>
auto Circuit::OutputTemplate<Const>::output_index() const noexcept -> connection_size_t {
    return output_index_;
}

template <bool Const>
auto Circuit::OutputTemplate<Const>::element() const -> ElementTemplate<Const> {
    return circuit_->element(element_id_);
}

template <bool Const>
auto Circuit::OutputTemplate<Const>::has_connected_element() const -> bool {
    return connected_element_id() != null_element;
}

template <bool Const>
auto Circuit::OutputTemplate<Const>::connected_element_id() const -> element_id_t {
    return connection_data_().element_id;
}

template <bool Const>
auto Circuit::OutputTemplate<Const>::connected_input_index() const -> connection_size_t {
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
void Circuit::OutputTemplate<Const>::clear_connection() const
    requires(!Const)
{
    auto &connection_data {connection_data_()};
    if (connection_data.element_id != null_element) {
        auto &destination_connection_data
            = circuit_->element_data_store_.at(connection_data.element_id)
                  .input_data.at(connection_data.index);

        // circuit_->input_data_store_.at(
        //     circuit_->element(connection_data.element_id)
        //     .input_id(connection_data.index));

        destination_connection_data.element_id = null_element;
        destination_connection_data.index = null_connection;

        connection_data.element_id = null_element;
        connection_data.index = null_connection;
    }
}

template <bool Const>
template <bool ConstOther>
void Circuit::OutputTemplate<Const>::connect(InputTemplate<ConstOther> input) const
    requires(!Const)
{
    clear_connection();

    // get data before we modify anything, for exception safety
    auto &connection_data {connection_data_()};
    auto &destination_connection_data
        = circuit_->element_data_store_.at(input.element_id())
              .input_data.at(input.input_index());
    // circuit_->input_data_store_.at(input.input_id());

    connection_data.element_id = input.element_id();
    connection_data.index = input.input_index();

    destination_connection_data.element_id = element_id();
    destination_connection_data.index = output_index();
}

template <bool Const>
auto Circuit::OutputTemplate<Const>::connection_data_() const -> ConnectionDataType & {
    // return circuit_->output_data_store_.at(output_id_);
    return circuit_->element_data_store_.at(element_id_).output_data.at(output_index_);
}

// Template Instanciations

template class Circuit::OutputTemplate<true>;
template class Circuit::OutputTemplate<false>;

template Circuit::OutputTemplate<true>::OutputTemplate(OutputTemplate<false>) noexcept;

template auto Circuit::OutputTemplate<false>::operator==<false>(
    OutputTemplate<false>) const noexcept -> bool;
template auto Circuit::OutputTemplate<false>::operator==<true>(
    OutputTemplate<true>) const noexcept -> bool;
template auto Circuit::OutputTemplate<true>::operator==<false>(
    OutputTemplate<false>) const noexcept -> bool;
template auto Circuit::OutputTemplate<true>::operator==<true>(
    OutputTemplate<true>) const noexcept -> bool;

template void Circuit::OutputTemplate<false>::connect<false>(InputTemplate<false>) const;
template void Circuit::OutputTemplate<false>::connect<true>(InputTemplate<true>) const;

}  // namespace logicsim

//
// Free Functions
//

namespace logicsim {

auto add_placeholder(Circuit::Output output) -> void {
    if (!output.has_connected_element()) {
        auto placeholder {output.circuit()->add_element(ElementType::placeholder, 1, 0)};
        output.connect(placeholder.input(0));
    }
}

auto add_element_placeholders(Circuit::Element element) -> void {
    std::ranges::for_each(element.outputs(), add_placeholder);
}

auto add_output_placeholders(Circuit &circuit) -> void {
    std::ranges::for_each(circuit.elements(), add_element_placeholders);
}

auto benchmark_circuit(const int n_elements) -> Circuit {
    Circuit circuit {};

    auto elem0 {circuit.add_element(ElementType::and_element, 2, 2)};

    for ([[maybe_unused]] auto count : range(n_elements - 1)) {
        auto wire0 = circuit.add_element(ElementType::wire, 1, 1);
        auto wire1 = circuit.add_element(ElementType::wire, 1, 1);
        auto elem1 = circuit.add_element(ElementType::and_element, 2, 2);

        elem0.output(0).connect(wire0.input(0));
        elem0.output(1).connect(wire1.input(0));

        wire0.output(0).connect(elem1.input(0));
        wire1.output(0).connect(elem1.input(1));

        elem0 = elem1;
    }

    return circuit;
}

namespace details {

template <std::uniform_random_bit_generator G>
void add_random_element(Circuit &circuit, G &rng) {
    constexpr connection_size_t max_connections {8};
    boost::random::uniform_int_distribution<connection_size_t> connection_dist {
        1, max_connections};
    boost::random::uniform_int_distribution<int8_t> element_dist {0, 2};

    const auto element_type {element_dist(rng) == 0
                                 ? ElementType::xor_element
                                 : (element_dist(rng) == 1 ? ElementType::inverter_element
                                                           : ElementType ::wire)};

    const connection_size_t one {1};
    const connection_size_t input_count {
        element_type == ElementType::xor_element ? connection_dist(rng) : one};

    const connection_size_t output_count {
        element_type == ElementType::wire ? connection_dist(rng) : one};

    circuit.add_element(element_type, input_count, output_count);
}

template <std::uniform_random_bit_generator G>
void create_random_elements(Circuit &circuit, G &rng, int n_elements) {
    for (auto _ [[maybe_unused]] : range(n_elements)) {
        add_random_element(circuit, rng);
    }
}

template <std::uniform_random_bit_generator G>
void create_random_connections(Circuit &circuit, G &rng, double connection_ratio) {
    if (connection_ratio == 0) {
        return;
    }
    if (connection_ratio < 0 || connection_ratio > 1) [[unlikely]] {
        throw_exception("connection ratio needs to be between 0 and 1.");
    }

    // collect inputs
    std::vector<Circuit::Input> all_inputs;
    all_inputs.reserve(circuit.input_count());
    for (auto element : circuit.elements()) {
        for (auto input : element.inputs()) {
            all_inputs.push_back(input);
        }
    }

    // collect outputs
    std::vector<Circuit::Output> all_outputs;
    all_outputs.reserve(circuit.output_count());
    for (auto element : circuit.elements()) {
        for (auto output : element.outputs()) {
            all_outputs.push_back(output);
        }
    }

    shuffle(all_inputs, rng);
    shuffle(all_outputs, rng);

    auto n_max_connections
        = gsl::narrow<double>(std::min(std::size(all_inputs), std::size(all_outputs)));
    auto n_connections
        = gsl::narrow<std::size_t>(std::round(connection_ratio * n_max_connections));

    for (auto index : range(n_connections)) {
        all_inputs.at(index).connect(all_outputs.at(index));
    }
}
}  // namespace details

template <std::uniform_random_bit_generator G>
auto create_random_circuit(G &rng, int n_elements, double connection_ratio) -> Circuit {
    Circuit circuit;
    details::create_random_elements(circuit, rng, n_elements);
    details::create_random_connections(circuit, rng, connection_ratio);

    return circuit;
}

// template instatiations

template auto create_random_circuit(boost::random::mt19937 &rng, int n_elements,
                                    double connection_ratio) -> Circuit;

}  // namespace logicsim
