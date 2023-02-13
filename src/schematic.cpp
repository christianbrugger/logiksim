
#include "schematic.h"

#include "algorithm.h"
#include "exceptions.h"
#include "format.h"
#include "iterator.h"
#include "line_tree.h"
#include "random.h"
#include "range.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <fmt/core.h>
#include <gsl/gsl>

#include <algorithm>
#include <utility>

namespace logicsim {

//
// Schematic
//

Schematic::Schematic(circuit_id_t circuit_id) : circuit_id_ {circuit_id} {
    if (circuit_id < null_circuit) [[unlikely]] {
        throw_exception("Schematic id of layout cannot be negative.");
    }
}

auto Schematic::swap(Schematic &other) noexcept -> void {
    using std::swap;

    element_data_store_.swap(other.element_data_store_);

    input_connections_.swap(other.input_connections_);
    output_connections_.swap(other.output_connections_);
    sub_circuit_ids_.swap(other.sub_circuit_ids_);
    element_types_.swap(other.element_types_);
    input_inverters_.swap(other.input_inverters_);
    output_delays_.swap(other.output_delays_);
    history_lengths_.swap(other.history_lengths_);

    swap(input_count_, other.input_count_);
    swap(output_count_, other.output_count_);
    swap(circuit_id_, other.circuit_id_);
}

auto swap(Schematic &a, Schematic &b) noexcept -> void {
    a.swap(b);
}

}  // namespace logicsim

template <>
auto std::swap(logicsim::Schematic &a, logicsim::Schematic &b) noexcept -> void {
    a.swap(b);
}

namespace logicsim {

auto Schematic::format() const -> std::string {
    std::string inner;
    if (!empty()) {
        auto format_true = [](auto element) { return element.format(true); };
        inner = fmt::format(": [\n  {}\n]", fmt_join(elements(), ",\n  ", format_true));
    }
    return fmt::format("<Schematic with {} elements{}>", element_count(), inner);
}

auto Schematic::circuit_id() const noexcept -> circuit_id_t {
    return circuit_id_;
}

auto Schematic::element_count() const noexcept -> std::size_t {
    return element_data_store_.size();
}

auto Schematic::empty() const noexcept -> bool {
    return element_data_store_.empty();
}

auto Schematic::is_element_id_valid(element_id_t element_id) const noexcept -> bool {
    auto size = gsl::narrow_cast<element_id_t::value_type>(element_count());
    return element_id.value >= 0 && element_id.value < size;
}

auto Schematic::element(element_id_t element_id) -> Element {
    if (!is_element_id_valid(element_id)) [[unlikely]] {
        throw_exception("Element id is invalid");
    }
    return Element {*this, element_id};
}

auto Schematic::element(element_id_t element_id) const -> ConstElement {
    if (!is_element_id_valid(element_id)) [[unlikely]] {
        throw_exception("Element id is invalid");
    }
    return ConstElement {*this, element_id};
}

auto Schematic::elements() noexcept -> ElementView {
    return ElementView {*this};
}

auto Schematic::elements() const noexcept -> ConstElementView {
    return ConstElementView {*this};
}

auto Schematic::add_element(ElementType type, std::size_t input_count,
                            std::size_t output_count) -> Element {
    return add_element(
        {.element_type = type, .input_count = input_count, .output_count = output_count});
}

auto Schematic::add_element(NewElementData &&data) -> Element {
    if (data.input_count < 0 || data.input_count > connection_id_t::max()) [[unlikely]] {
        throw_exception("Input count needs to be positive and not too large.");
    }
    if (data.output_count < 0 || data.output_count > connection_id_t::max())
        [[unlikely]] {
        throw_exception("Output count needs to be positive and not too large.");
    }

    // make sure we can represent all ids
    if (element_data_store_.size() + 1 >= element_id_t::max()) [[unlikely]] {
        throw_exception("Reached maximum number of elements.");
    }
    if (input_count_ >= std::numeric_limits<decltype(input_count_)>::max()
                            - data.input_count) [[unlikely]] {
        throw_exception("Reached maximum number of inputs.");
    }
    if (output_count_ >= std::numeric_limits<decltype(input_count_)>::max()
                             - data.output_count) [[unlikely]] {
        throw_exception("Reached maximum number of outputs.");
    }

    element_data_store_.push_back({
        .input_data = {},
        .output_data = {},
        .type = data.element_type,
    });

    // extend vectors
    element_types_.push_back(data.element_type);
    sub_circuit_ids_.push_back(data.circuit_id);
    input_connections_.emplace_back(data.input_count, ConnectionData {});
    output_connections_.emplace_back(data.output_count, ConnectionData {});
    if (data.input_inverters.size() == 0) {
        input_inverters_.emplace_back(data.input_count, false);
    } else {
        if (std::size(data.input_inverters) != data.input_count) [[unlikely]] {
            throw_exception("Need as many values for input_inverters as inputs.");
        }
        input_inverters_.emplace_back(std::begin(data.input_inverters),
                                      std::end(data.input_inverters));
    }
    if (data.output_delays.size() == 0) {
        output_delays_.emplace_back(data.output_count, defaults::standard_delay);
    } else {
        if (std::size(data.output_delays) != data.output_count) [[unlikely]] {
            throw_exception("Need as many output_delays as outputs.");
        }
        output_delays_.emplace_back(std::begin(data.output_delays),
                                    std::end(data.output_delays));
    }
    history_lengths_.push_back(data.history_length);

    //
    auto element_id = element_id_t {gsl::narrow_cast<element_id_t::value_type>(
        element_data_store_.size() - std::size_t {1})};
    element_data_store_.at(element_id.value).input_data.resize(data.input_count);
    element_data_store_.at(element_id.value).output_data.resize(data.output_count);

    input_count_ += data.input_count;
    output_count_ += data.output_count;

    return element(element_id);
}

auto Schematic::clear() -> void {
    element_data_store_.clear();
    input_count_ = 0;
    output_count_ = 0;
}

auto Schematic::input_count() const noexcept -> std::size_t {
    return input_count_;
}

auto Schematic::output_count() const noexcept -> std::size_t {
    return output_count_;
}

auto validate_output_connected(const Schematic::ConstOutput output) -> void {
    if (!output.has_connected_element()) [[unlikely]] {
        throw_exception("Element has unconnected output.");
    }
}

auto validate_outputs_connected(const Schematic::ConstElement element) -> void {
    std::ranges::for_each(element.outputs(), validate_output_connected);
}

auto validate_input_consistent(const Schematic::ConstInput input) -> void {
    if (input.has_connected_element()) {
        auto back_reference {input.connected_output().connected_input()};
        if (back_reference != input) [[unlikely]] {
            throw_exception("Back reference doesn't match.");
        }
    }
}

auto validate_output_consistent(const Schematic::ConstOutput output) -> void {
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

auto validate_element_connections_consistent(const Schematic::ConstElement element)
    -> void {
    std::ranges::for_each(element.inputs(), validate_input_consistent);
    std::ranges::for_each(element.outputs(), validate_output_consistent);
}

auto Schematic::validate_connection_data_(const Schematic::ConnectionData connection_data)
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

auto Schematic::validate(bool require_all_outputs_connected) const -> void {
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
    // std::ranges::for_each(input_data_store_, Schematic::validate_connection_data_);
    // std::ranges::for_each(output_data_store_,
    // Schematic::validate_connection_data_);

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
Schematic::ElementIteratorTemplate<Const>::ElementIteratorTemplate(
    schematic_type &schematic, element_id_t element_id) noexcept
    : schematic_(&schematic), element_id_(element_id) {}

template <bool Const>
auto Schematic::ElementIteratorTemplate<Const>::operator*() const -> value_type {
    if (schematic_ == nullptr) [[unlikely]] {
        throw_exception("schematic cannot be null when dereferencing element iterator");
    }
    return schematic_->element(element_id_);
}

template <bool Const>
auto Schematic::ElementIteratorTemplate<Const>::operator++() noexcept
    -> Schematic::ElementIteratorTemplate<Const> & {
    ++element_id_.value;
    return *this;
}

template <bool Const>
auto Schematic::ElementIteratorTemplate<Const>::operator++(int) noexcept
    -> Schematic::ElementIteratorTemplate<Const> {
    auto tmp = *this;
    ++(*this);
    return tmp;
}

template <bool Const>
auto Schematic::ElementIteratorTemplate<Const>::operator==(
    const ElementIteratorTemplate &right) const noexcept -> bool {
    return element_id_ >= right.element_id_;
}

template <bool Const>
auto Schematic::ElementIteratorTemplate<Const>::operator-(
    const ElementIteratorTemplate &right) const noexcept -> difference_type {
    return element_id_.value - right.element_id_.value;
}

template class Schematic::ElementIteratorTemplate<false>;
template class Schematic::ElementIteratorTemplate<true>;

//
// Element View
//

template <bool Const>
Schematic::ElementViewTemplate<Const>::ElementViewTemplate(
    schematic_type &schematic) noexcept
    : schematic_(&schematic) {}

template <bool Const>
auto Schematic::ElementViewTemplate<Const>::begin() const noexcept -> iterator_type {
    return iterator_type {*schematic_, element_id_t {0}};
}

template <bool Const>
auto Schematic::ElementViewTemplate<Const>::end() const noexcept -> iterator_type {
    return iterator_type {*schematic_,
                          element_id_t {gsl::narrow_cast<element_id_t::value_type>(
                              schematic_->element_count())}};
}

template <bool Const>
auto Schematic::ElementViewTemplate<Const>::size() const noexcept -> std::size_t {
    return schematic_->element_count();
}

template <bool Const>
auto Schematic::ElementViewTemplate<Const>::empty() const noexcept -> bool {
    return schematic_->empty();
}

template class Schematic::ElementViewTemplate<false>;
template class Schematic::ElementViewTemplate<true>;

static_assert(std::ranges::input_range<Schematic::ElementView>);
static_assert(std::ranges::input_range<Schematic::ConstElementView>);

//
// Schematic::Element
//

template <bool Const>
Schematic::ElementTemplate<Const>::ElementTemplate(SchematicType &schematic,
                                                   element_id_t element_id) noexcept
    : schematic_(&schematic), element_id_(element_id) {}

template <bool Const>
template <bool ConstOther>
Schematic::ElementTemplate<Const>::ElementTemplate(
    ElementTemplate<ConstOther> element) noexcept
    requires Const && (!ConstOther)
    : schematic_(element.schematic_), element_id_(element.element_id_) {}

template <bool Const>
Schematic::ElementTemplate<Const>::operator element_id_t() const noexcept {
    return element_id_;
}

template <bool Const>
template <bool ConstOther>
auto Schematic::ElementTemplate<Const>::operator==(
    ElementTemplate<ConstOther> other) const noexcept -> bool {
    return schematic_ == other.schematic_ && element_id_ == other.element_id_;
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::format(bool with_connections) const
    -> std::string {
    auto connections = !with_connections
                           ? ""
                           : fmt::format(", inputs = {}, outputs = {}", inputs().format(),
                                         outputs().format());

    return fmt::format("<Element {}: {}x{} {}{}>", element_id(), input_count(),
                       output_count(), element_type(), connections);
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::schematic() const noexcept -> SchematicType * {
    return schematic_;
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::element_id() const noexcept -> element_id_t {
    return element_id_;
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::element_type() const -> ElementType {
    return element_data_().type;
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::input_count() const -> std::size_t {
    return element_data_().input_data.size();
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::output_count() const -> std::size_t {
    return element_data_().output_data.size();
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::input(connection_id_t input) const
    -> InputTemplate<Const> {
    return InputTemplate<Const> {*schematic_, element_id_, input};
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::output(connection_id_t output) const
    -> OutputTemplate<Const> {
    return OutputTemplate<Const> {*schematic_, element_id_, output};
}

template <bool Const>
inline auto Schematic::ElementTemplate<Const>::inputs() const
    -> InputViewTemplate<Const> {
    return InputViewTemplate<Const> {*this};
}

template <bool Const>
inline auto Schematic::ElementTemplate<Const>::outputs() const
    -> OutputViewTemplate<Const> {
    return OutputViewTemplate<Const> {*this};
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::element_data_() const -> ElementDataType & {
    return schematic_->element_data_store_.at(element_id_.value);
}

// Template Instanciations

template class Schematic::ElementTemplate<true>;
template class Schematic::ElementTemplate<false>;

template Schematic::ElementTemplate<true>::ElementTemplate(
    ElementTemplate<false>) noexcept;

template auto Schematic::ElementTemplate<false>::operator==<false>(
    ElementTemplate<false>) const noexcept -> bool;
template auto Schematic::ElementTemplate<false>::operator==<true>(
    ElementTemplate<true>) const noexcept -> bool;
template auto Schematic::ElementTemplate<true>::operator==<false>(
    ElementTemplate<false>) const noexcept -> bool;
template auto Schematic::ElementTemplate<true>::operator==<true>(
    ElementTemplate<true>) const noexcept -> bool;

//
// Connection Iterator
//

template <bool Const, bool IsInput>
auto Schematic::ConnectionIteratorTemplate<Const, IsInput>::operator*() const
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
auto Schematic::ConnectionIteratorTemplate<Const, IsInput>::operator++() noexcept
    -> Schematic::ConnectionIteratorTemplate<Const, IsInput> & {
    ++connection_id.value;
    return *this;
}

template <bool Const, bool IsInput>
auto Schematic::ConnectionIteratorTemplate<Const, IsInput>::operator++(int) noexcept
    -> ConnectionIteratorTemplate {
    auto tmp = *this;
    ++(*this);
    return tmp;
}

template <bool Const, bool IsInput>
auto Schematic::ConnectionIteratorTemplate<Const, IsInput>::operator==(
    const ConnectionIteratorTemplate &right) const noexcept -> bool {
    return connection_id >= right.connection_id;
}

template <bool Const, bool IsInput>
auto Schematic::ConnectionIteratorTemplate<Const, IsInput>::operator-(
    const ConnectionIteratorTemplate &right) const noexcept -> difference_type {
    return connection_id.value - right.connection_id.value;
}

template class Schematic::ConnectionIteratorTemplate<false, false>;
template class Schematic::ConnectionIteratorTemplate<true, false>;
template class Schematic::ConnectionIteratorTemplate<false, true>;
template class Schematic::ConnectionIteratorTemplate<true, true>;

//
// Connection View
//

template <bool Const, bool IsInput>
Schematic::ConnectionViewTemplate<Const, IsInput>::ConnectionViewTemplate(
    ElementTemplate<Const> element) noexcept
    : element_(element) {}

template <bool Const, bool IsInput>
auto Schematic::ConnectionViewTemplate<Const, IsInput>::begin() const -> iterator_type {
    return iterator_type {element_, connection_id_t {0}};
}

template <bool Const, bool IsInput>
auto Schematic::ConnectionViewTemplate<Const, IsInput>::end() const -> iterator_type {
    return iterator_type {
        element_,
        connection_id_t {gsl::narrow_cast<connection_id_t::value_type>(size())}};
}

template <bool Const, bool IsInput>
auto Schematic::ConnectionViewTemplate<Const, IsInput>::size() const -> std::size_t {
    if constexpr (IsInput) {
        return element_.input_count();
    } else {
        return element_.output_count();
    }
}

template <bool Const, bool IsInput>
auto Schematic::ConnectionViewTemplate<Const, IsInput>::empty() const -> bool {
    return size() == 0;
}

template <bool Const, bool IsInput>
auto Schematic::ConnectionViewTemplate<Const, IsInput>::format() const -> std::string {
    auto connections = transform_view(*this, &value_type::format_connection);
    return fmt::format("{}", connections);
}

template class Schematic::ConnectionViewTemplate<false, false>;
template class Schematic::ConnectionViewTemplate<true, false>;
template class Schematic::ConnectionViewTemplate<false, true>;
template class Schematic::ConnectionViewTemplate<true, true>;

static_assert(std::ranges::input_range<Schematic::InputView>);
static_assert(std::ranges::input_range<Schematic::ConstInputView>);
static_assert(std::ranges::input_range<Schematic::OutputView>);
static_assert(std::ranges::input_range<Schematic::ConstOutputView>);

//
// Schematic::Input
//

template <bool Const>
Schematic::InputTemplate<Const>::InputTemplate(SchematicType &schematic,
                                               element_id_t element_id,
                                               connection_id_t input_index) noexcept
    : schematic_(&schematic), element_id_(element_id), input_index_(input_index) {}

template <bool Const>
template <bool ConstOther>
Schematic::InputTemplate<Const>::InputTemplate(InputTemplate<ConstOther> input) noexcept
    requires Const && (!ConstOther)
    : schematic_(input.schematic_),
      element_id_(input.element_id_),
      input_index_(input.input_index_) {}

template <bool Const>
template <bool ConstOther>
auto Schematic::InputTemplate<Const>::operator==(
    InputTemplate<ConstOther> other) const noexcept -> bool {
    return schematic_ == other.schematic_ && element_id_ == other.element_id_
           && input_index_ == other.input_index_;
}

template <bool Const>
auto Schematic::InputTemplate<Const>::format() const -> std::string {
    const auto element = this->element();
    return fmt::format("<Input {} of Element {}: {} {} x {}>", input_index(),
                       element_id(), element.element_type(), element.input_count(),
                       element.output_count());
}

template <bool Const>
auto Schematic::InputTemplate<Const>::format_connection() const -> std::string {
    if (has_connected_element()) {
        return fmt::format("Element_{}-{}", connected_element_id(),
                           connected_output_index());
    }
    return "---";
}

template <bool Const>
auto Schematic::InputTemplate<Const>::schematic() const noexcept -> SchematicType * {
    return schematic_;
}

template <bool Const>
auto Schematic::InputTemplate<Const>::element_id() const noexcept -> element_id_t {
    return element_id_;
}

template <bool Const>
auto Schematic::InputTemplate<Const>::input_index() const noexcept -> connection_id_t {
    return input_index_;
}

template <bool Const>
auto Schematic::InputTemplate<Const>::element() const -> ElementTemplate<Const> {
    return schematic_->element(element_id_);
}

template <bool Const>
auto Schematic::InputTemplate<Const>::has_connected_element() const -> bool {
    return connected_element_id() != null_element;
}

template <bool Const>
auto Schematic::InputTemplate<Const>::connected_element_id() const -> element_id_t {
    return connection_data_().element_id;
}

template <bool Const>
auto Schematic::InputTemplate<Const>::connected_output_index() const -> connection_id_t {
    return connection_data_().index;
}

template <bool Const>
auto Schematic::InputTemplate<Const>::connected_element() const
    -> ElementTemplate<Const> {
    return schematic_->element(connected_element_id());
}

template <bool Const>
auto Schematic::InputTemplate<Const>::connected_output() const -> OutputTemplate<Const> {
    return connected_element().output(connected_output_index());
}

template <bool Const>
void Schematic::InputTemplate<Const>::clear_connection() const
    requires(!Const)
{
    auto &connection_data {connection_data_()};
    if (connection_data.element_id != null_element) {
        auto &destination_connection_data {
            schematic_->element_data_store_.at(connection_data.element_id.value)
                .output_data.at(connection_data.index.value)};

        destination_connection_data.element_id = null_element;
        destination_connection_data.index = null_connection;

        connection_data.element_id = null_element;
        connection_data.index = null_connection;
    }
}

template <bool Const>
template <bool ConstOther>
void Schematic::InputTemplate<Const>::connect(OutputTemplate<ConstOther> output) const
    requires(!Const)
{
    clear_connection();

    // get data before we modify anything, for exception safety
    auto &destination_connection_data
        = schematic_->element_data_store_.at(output.element_id().value)
              .output_data.at(output.output_index().value);

    auto &connection_data {connection_data_()};

    connection_data.element_id = output.element_id();
    connection_data.index = output.output_index();

    destination_connection_data.element_id = element_id();
    destination_connection_data.index = input_index();
}

template <bool Const>
auto Schematic::InputTemplate<Const>::connection_data_() const -> ConnectionDataType & {
    // return schematic_->input_data_store_.at(input_id_);
    return schematic_->element_data_store_.at(element_id_.value)
        .input_data.at(input_index_.value);
}

// Template Instanciations

template class Schematic::InputTemplate<true>;
template class Schematic::InputTemplate<false>;

template Schematic::InputTemplate<true>::InputTemplate(InputTemplate<false>) noexcept;

template auto Schematic::InputTemplate<false>::operator==<false>(
    InputTemplate<false>) const noexcept -> bool;
template auto Schematic::InputTemplate<false>::operator==<true>(
    InputTemplate<true>) const noexcept -> bool;
template auto Schematic::InputTemplate<true>::operator==<false>(
    InputTemplate<false>) const noexcept -> bool;
template auto Schematic::InputTemplate<true>::operator==<true>(
    InputTemplate<true>) const noexcept -> bool;

template void Schematic::InputTemplate<false>::connect<false>(
    OutputTemplate<false>) const;
template void Schematic::InputTemplate<false>::connect<true>(OutputTemplate<true>) const;

//
// Schematic::Output
//

template <bool Const>
Schematic::OutputTemplate<Const>::OutputTemplate(SchematicType &schematic,
                                                 element_id_t element_id,
                                                 connection_id_t output_index) noexcept
    : schematic_(&schematic), element_id_(element_id), output_index_(output_index) {}

template <bool Const>
template <bool ConstOther>
Schematic::OutputTemplate<Const>::OutputTemplate(
    OutputTemplate<ConstOther> output) noexcept
    requires Const && (!ConstOther)
    : schematic_(output.schematic_),
      element_id_(output.element_id_),
      output_index_(output.output_index_) {}

template <bool Const>
template <bool ConstOther>
auto Schematic::OutputTemplate<Const>::operator==(
    Schematic::OutputTemplate<ConstOther> other) const noexcept -> bool {
    return schematic_ == other.schematic_ && element_id_ == other.element_id_
           && output_index_ == other.output_index_;
}

template <bool Const>
auto Schematic::OutputTemplate<Const>::format() const -> std::string {
    const auto element = this->element();
    return fmt::format("<Output {} of Element {}: {} {} x {}>", output_index(),
                       element_id(), element.element_type(), element.input_count(),
                       element.output_count());
}

template <bool Const>
auto Schematic::OutputTemplate<Const>::format_connection() const -> std::string {
    if (has_connected_element()) {
        return fmt::format("Element_{}-{}", connected_element_id(),
                           connected_input_index());
    }
    return "---";
}

template <bool Const>
auto Schematic::OutputTemplate<Const>::schematic() const noexcept -> SchematicType * {
    return schematic_;
}

template <bool Const>
auto Schematic::OutputTemplate<Const>::element_id() const noexcept -> element_id_t {
    return element_id_;
}

template <bool Const>
auto Schematic::OutputTemplate<Const>::output_index() const noexcept -> connection_id_t {
    return output_index_;
}

template <bool Const>
auto Schematic::OutputTemplate<Const>::element() const -> ElementTemplate<Const> {
    return schematic_->element(element_id_);
}

template <bool Const>
auto Schematic::OutputTemplate<Const>::has_connected_element() const -> bool {
    return connected_element_id() != null_element;
}

template <bool Const>
auto Schematic::OutputTemplate<Const>::connected_element_id() const -> element_id_t {
    return connection_data_().element_id;
}

template <bool Const>
auto Schematic::OutputTemplate<Const>::connected_input_index() const -> connection_id_t {
    return connection_data_().index;
}

template <bool Const>
auto Schematic::OutputTemplate<Const>::connected_element() const
    -> ElementTemplate<Const> {
    return schematic_->element(connected_element_id());
}

template <bool Const>
auto Schematic::OutputTemplate<Const>::connected_input() const -> InputTemplate<Const> {
    return connected_element().input(connected_input_index());
}

template <bool Const>
void Schematic::OutputTemplate<Const>::clear_connection() const
    requires(!Const)
{
    auto &connection_data {connection_data_()};
    if (connection_data.element_id != null_element) {
        auto &destination_connection_data
            = schematic_->element_data_store_.at(connection_data.element_id.value)
                  .input_data.at(connection_data.index.value);

        destination_connection_data.element_id = null_element;
        destination_connection_data.index = null_connection;

        connection_data.element_id = null_element;
        connection_data.index = null_connection;
    }
}

template <bool Const>
template <bool ConstOther>
void Schematic::OutputTemplate<Const>::connect(InputTemplate<ConstOther> input) const
    requires(!Const)
{
    clear_connection();

    // get data before we modify anything, for exception safety
    auto &connection_data {connection_data_()};
    auto &destination_connection_data
        = schematic_->element_data_store_.at(input.element_id().value)
              .input_data.at(input.input_index().value);
    // schematic_->input_data_store_.at(input.input_id());

    connection_data.element_id = input.element_id();
    connection_data.index = input.input_index();

    destination_connection_data.element_id = element_id();
    destination_connection_data.index = output_index();
}

template <bool Const>
auto Schematic::OutputTemplate<Const>::connection_data_() const -> ConnectionDataType & {
    // return schematic_->output_data_store_.at(output_id_);
    return schematic_->element_data_store_.at(element_id_.value)
        .output_data.at(output_index_.value);
}

// Template Instanciations

template class Schematic::OutputTemplate<true>;
template class Schematic::OutputTemplate<false>;

template Schematic::OutputTemplate<true>::OutputTemplate(OutputTemplate<false>) noexcept;

template auto Schematic::OutputTemplate<false>::operator==<false>(
    OutputTemplate<false>) const noexcept -> bool;
template auto Schematic::OutputTemplate<false>::operator==<true>(
    OutputTemplate<true>) const noexcept -> bool;
template auto Schematic::OutputTemplate<true>::operator==<false>(
    OutputTemplate<false>) const noexcept -> bool;
template auto Schematic::OutputTemplate<true>::operator==<true>(
    OutputTemplate<true>) const noexcept -> bool;

template void Schematic::OutputTemplate<false>::connect<false>(
    InputTemplate<false>) const;
template void Schematic::OutputTemplate<false>::connect<true>(InputTemplate<true>) const;

}  // namespace logicsim

//
// Free Functions
//

namespace logicsim {

auto add_placeholder(Schematic::Output output) -> void {
    if (!output.has_connected_element()) {
        auto placeholder {
            output.schematic()->add_element(ElementType::placeholder, 1, 0)};
        output.connect(placeholder.input(connection_id_t {0}));
    }
}

auto add_element_placeholders(Schematic::Element element) -> void {
    std::ranges::for_each(element.outputs(), add_placeholder);
}

auto calculate_output_delays(const LineTree &line_tree) -> std::vector<delay_t> {
    auto lengths = line_tree.calculate_output_lengths();
    return transform_to_vector(lengths, [](LineTree::length_t length) -> delay_t {
        // TODO handle overflow
        return delay_t {Schematic::defaults::wire_delay_per_distance.value * length};
    });
}

auto add_output_placeholders(Schematic &schematic) -> void {
    std::ranges::for_each(schematic.elements(), add_element_placeholders);
}

auto benchmark_schematic(const int n_elements) -> Schematic {
    Schematic schematic {};

    auto elem0 {schematic.add_element(ElementType::and_element, 2, 2)};

    for ([[maybe_unused]] auto count : range(n_elements - 1)) {
        auto wire0 = schematic.add_element(ElementType::wire, 1, 1);
        auto wire1 = schematic.add_element(ElementType::wire, 1, 1);
        auto elem1 = schematic.add_element(ElementType::and_element, 2, 2);

        elem0.output(connection_id_t {0}).connect(wire0.input(connection_id_t {0}));
        elem0.output(connection_id_t {1}).connect(wire1.input(connection_id_t {0}));

        wire0.output(connection_id_t {0}).connect(elem1.input(connection_id_t {0}));
        wire1.output(connection_id_t {0}).connect(elem1.input(connection_id_t {1}));

        elem0 = elem1;
    }

    return schematic;
}

namespace details {

template <std::uniform_random_bit_generator G>
void add_random_element(Schematic &schematic, G &rng) {
    static constexpr auto max_connections = 8;
    auto connection_dist
        = boost::random::uniform_int_distribution<int> {1, max_connections};
    auto element_dist = boost::random::uniform_int_distribution<int8_t> {0, 2};

    const auto element_type
        = element_dist(rng) == 0 ? ElementType::xor_element
                                 : (element_dist(rng) == 1 ? ElementType::inverter_element
                                                           : ElementType ::wire);

    const auto input_count
        = element_type == ElementType::xor_element ? connection_dist(rng) : 1;

    const auto output_count
        = element_type == ElementType::wire ? connection_dist(rng) : 1;

    schematic.add_element(element_type, input_count, output_count);
}

template <std::uniform_random_bit_generator G>
void create_random_elements(Schematic &schematic, G &rng, int n_elements) {
    for (auto _ [[maybe_unused]] : range(n_elements)) {
        add_random_element(schematic, rng);
    }
}

template <std::uniform_random_bit_generator G>
void create_random_connections(Schematic &schematic, G &rng, double connection_ratio) {
    if (connection_ratio == 0) {
        return;
    }
    if (connection_ratio < 0 || connection_ratio > 1) [[unlikely]] {
        throw_exception("connection ratio needs to be between 0 and 1.");
    }

    // collect inputs
    std::vector<Schematic::Input> all_inputs;
    all_inputs.reserve(schematic.input_count());
    for (auto element : schematic.elements()) {
        for (auto input : element.inputs()) {
            all_inputs.push_back(input);
        }
    }

    // collect outputs
    std::vector<Schematic::Output> all_outputs;
    all_outputs.reserve(schematic.output_count());
    for (auto element : schematic.elements()) {
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
auto create_random_schematic(G &rng, int n_elements, double connection_ratio)
    -> Schematic {
    Schematic schematic;
    details::create_random_elements(schematic, rng, n_elements);
    details::create_random_connections(schematic, rng, connection_ratio);

    return schematic;
}

// template instatiations

template auto create_random_schematic(boost::random::mt19937 &rng, int n_elements,
                                      double connection_ratio) -> Schematic;

}  // namespace logicsim
