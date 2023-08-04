
#include "schematic.h"

#include "algorithm.h"
#include "exceptions.h"
#include "format.h"
#include "iterator_adaptor.h"
#include "layout_calculations.h"
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

Schematic::Schematic(circuit_id_t circuit_id) : circuit_id_ {circuit_id} {}

Schematic::Schematic(delay_t wire_delay_per_distance)
    : wire_delay_per_distance_ {wire_delay_per_distance} {}

Schematic::Schematic(circuit_id_t circuit_id, delay_t wire_delay_per_distance)
    : circuit_id_ {circuit_id}, wire_delay_per_distance_ {wire_delay_per_distance} {}

auto Schematic::clear() -> void {
    input_connections_.clear();
    output_connections_.clear();
    sub_circuit_ids_.clear();
    element_types_.clear();
    input_inverters_.clear();
    output_delays_.clear();
    history_lengths_.clear();

    input_count_ = 0;
    output_count_ = 0;
}

auto Schematic::swap(Schematic &other) noexcept -> void {
    using std::swap;

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

auto Schematic::swap_element_data(element_id_t element_id_1, element_id_t element_id_2,
                                  bool update_connections) -> void {
    if (element_id_1 == element_id_2) {
        return;
    }

    const auto swap_ids = [element_id_1, element_id_2](auto &container) {
        using std::swap;
        swap(container.at(element_id_1.value), container.at(element_id_2.value));
    };

    swap_ids(input_connections_);
    swap_ids(output_connections_);
    swap_ids(sub_circuit_ids_);
    swap_ids(element_types_);
    swap_ids(input_inverters_);
    swap_ids(output_delays_);
    swap_ids(history_lengths_);

    if (update_connections) {
        update_swapped_connections(element_id_1, element_id_2);
        update_swapped_connections(element_id_2, element_id_1);
    }
}

auto Schematic::delete_last_element(bool clear_connections) -> void {
    if (input_connections_.empty()      //
        || output_connections_.empty()  //
        || sub_circuit_ids_.empty()     //
        || element_types_.empty()       //
        || input_inverters_.empty()     //
        || output_delays_.empty()       //
        || history_lengths_.empty()     //
        ) [[unlikely]] {
        throw_exception("Cannot delete last element of empty schematics.");
    }

    if (clear_connections) {
        const auto last_id = element_id_t {
            gsl::narrow_cast<element_id_t::value_type>(element_count() - 1)};
        element(last_id).clear_all_connection();
    }

    const auto last_input_count = input_connections_.back().size();
    const auto last_output_count = output_connections_.back().size();
    if (input_count_ < last_input_count || output_count_ < last_output_count)
        [[unlikely]] {
        throw_exception("input or output count underflows");
    }
    input_count_ -= last_input_count;
    output_count_ -= last_output_count;

    // delete
    input_connections_.pop_back();
    output_connections_.pop_back();
    sub_circuit_ids_.pop_back();
    element_types_.pop_back();
    input_inverters_.pop_back();
    output_delays_.pop_back();
    history_lengths_.pop_back();
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
    std::string inner {};
    if (!empty()) {
        auto format_true = [](auto element) { return element.format(true); };
        inner = fmt::format(": [\n  {}\n]",
                            fmt_join(",\n  ", elements(), "{}", format_true));
    }
    return fmt::format("<Schematic with {} elements{}>", element_count(), inner);
}

auto Schematic::circuit_id() const noexcept -> circuit_id_t {
    return circuit_id_;
}

auto Schematic::element_count() const noexcept -> std::size_t {
    return element_types_.size();
}

auto Schematic::empty() const noexcept -> bool {
    return element_types_.empty();
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

auto Schematic::input(connection_t connection) -> Input {
    return element(connection.element_id).input(connection.connection_id);
}

auto Schematic::input(connection_t connection) const -> ConstInput {
    return element(connection.element_id).input(connection.connection_id);
}

auto Schematic::output(connection_t connection) -> Output {
    return element(connection.element_id).output(connection.connection_id);
}

auto Schematic::output(connection_t connection) const -> ConstOutput {
    return element(connection.element_id).output(connection.connection_id);
}

auto Schematic::wire_delay_per_distance() const -> delay_t {
    return wire_delay_per_distance_;
}

auto Schematic::add_element(ElementData &&data) -> Element {
    if (data.input_count > connection_id_t::max()) [[unlikely]] {
        throw_exception("Input count needs to be positive and not too large.");
    }
    if (data.output_count > connection_id_t::max()) [[unlikely]] {
        throw_exception("Output count needs to be positive and not too large.");
    }

    // make sure we can represent all ids
    if (element_types_.size() + 1 >= element_id_t::max()) [[unlikely]] {
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

    // extend vectors
    element_types_.push_back(data.element_type);
    sub_circuit_ids_.push_back(data.circuit_id);
    input_connections_.emplace_back(data.input_count,
                                    connection_t {null_element, null_connection});
    output_connections_.emplace_back(data.output_count,
                                     connection_t {null_element, null_connection});
    if (data.input_inverters.size() == 0) {
        input_inverters_.emplace_back(data.input_count, false);
    } else {
        if (data.input_inverters.size() != data.input_count) [[unlikely]] {
            throw_exception("Need as many values for input_inverters as inputs.");
        }
        input_inverters_.emplace_back(data.input_inverters);
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

    // return element
    auto element_id = element_id_t {gsl::narrow_cast<element_id_t::value_type>(
        element_types_.size() - std::size_t {1})};

    input_count_ += data.input_count;
    output_count_ += data.output_count;

    return element(element_id);
}

auto Schematic::swap_and_delete_element(element_id_t element_id) -> element_id_t {
    const auto last_element_id = element_id_t {
        gsl::narrow_cast<element_id_t::value_type>(element_count() - std::size_t {1})};

    element(element_id).clear_all_connection();

    if (element_id != last_element_id) {
        swap_element_data(element_id, last_element_id, false);
        update_swapped_connections(element_id, last_element_id);
    }

    delete_last_element(false);
    return last_element_id;
}

auto Schematic::swap_elements(element_id_t element_id_0, element_id_t element_id_1)
    -> void {
    swap_element_data(element_id_0, element_id_1, true);
}

auto Schematic::update_swapped_connections(element_id_t new_element_id,
                                           element_id_t old_element_id) -> void {
    if (new_element_id == old_element_id) {
        return;
    }

    const auto old_element = element(old_element_id);
    const auto new_element = element(new_element_id);

    for (auto input : new_element.inputs()) {
        if (input.has_connected_element()) {
            if (input.connected_element_id() == old_element_id) {
                // self connection?
                input.connect(new_element.output(input.connected_output_index()));
            } else if (input.connected_element_id() == new_element_id) {
                // swapped connection?
                input.connect(old_element.output(input.connected_output_index()));
            } else {
                // fix back connection
                input.connect(input.connected_output());
            }
        }
    }

    for (auto output : new_element.outputs()) {
        if (output.has_connected_element()) {
            if (output.connected_element_id() == old_element_id) {
                // self connection?
                output.connect(new_element.input(output.connected_input_index()));
            } else if (output.connected_element_id() == new_element_id) {
                // swapped connection?
                output.connect(old_element.input(output.connected_input_index()));
            } else {
                // fix back connection
                output.connect(output.connected_input());
            }
        }
    }
}

auto Schematic::input_count() const noexcept -> std::size_t {
    return input_count_;
}

auto Schematic::output_count() const noexcept -> std::size_t {
    return output_count_;
}

auto validate_input_connected(const Schematic::ConstInput input) -> void {
    if (!input.has_connected_element()) [[unlikely]] {
        throw_exception("Element has unconnected input.");
    }
}

auto validate_output_connected(const Schematic::ConstOutput output) -> void {
    if (!output.has_connected_element()) [[unlikely]] {
        throw_exception("Element has unconnected output.");
    }
}

auto validate_input_disconnected(const Schematic::ConstInput input) -> void {
    if (input.has_connected_element()) [[unlikely]] {
        throw_exception("Element has connected input.");
    }
}

auto validate_output_disconnected(const Schematic::ConstOutput output) -> void {
    if (output.has_connected_element()) [[unlikely]] {
        throw_exception("Element has connected output.");
    }
}

auto validate_all_outputs_connected(const Schematic::ConstElement element) -> void {
    std::ranges::for_each(element.outputs(), validate_output_connected);
}

auto validate_all_inputs_disconnected(const Schematic::ConstElement element) -> void {
    std::ranges::for_each(element.inputs(), validate_input_disconnected);
}

auto validate_all_outputs_disconnected(const Schematic::ConstElement element) -> void {
    std::ranges::for_each(element.outputs(), validate_output_disconnected);
}

auto validate_placeholder_connected(const Schematic::ConstElement element) -> void {
    if (element.element_type() == ElementType::placeholder) {
        std::ranges::for_each(element.inputs(), validate_input_connected);
        std::ranges::for_each(element.outputs(), validate_output_connected);
    }
}

auto validate_has_no_placeholders(const Schematic::ConstElement element) -> void {
    const auto is_placeholder = [](const Schematic::ConstOutput output) {
        return output.has_connected_element()
               && output.connected_element().is_placeholder();
    };
    if (std::ranges::any_of(element.outputs(), is_placeholder)) {
        throw_exception("element should not have output placeholders");
    }
}

auto validate_input_consistent(const Schematic::ConstInput input) -> void {
    if (input.has_connected_element()) {
        if (!input.connected_output().has_connected_element()) [[unlikely]] {
            throw_exception("Back reference is missing.");
        }
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

auto validate_no_input_loops(const Schematic::ConstInput input) -> void {
    // clocks have an internal loop, that's allowed.
    const auto clock_loop = [=]() {
        return input.element().element_type() == ElementType::clock_generator
               && input.input_index() == connection_id_t {0}
               && input.connected_output_index() == connection_id_t {0};
    };

    if (input.connected_element_id() == input.element_id() && !clock_loop())
        [[unlikely]] {
        throw_exception("element connects to itself, loops are not allowed.");
    }
}

auto validate_no_output_loops(const Schematic::ConstOutput output) -> void {
    // clocks have an internal loop, that's allowed.
    const auto clock_loop = [=]() {
        return output.element().element_type() == ElementType::clock_generator
               && output.output_index() == connection_id_t {0}
               && output.connected_input_index() == connection_id_t {0};
    };

    if (output.connected_element_id() == output.element_id() && !clock_loop())
        [[unlikely]] {
        throw_exception("element connects to itself, loops are not allowed.");
    }
}

auto validate_element_connections_no_loops(const Schematic::ConstElement element)
    -> void {
    std::ranges::for_each(element.inputs(), validate_no_input_loops);
    std::ranges::for_each(element.outputs(), validate_no_output_loops);
}

auto validate_input_output_count(const Schematic::ConstElement element) -> void {
    if (!is_input_output_count_valid(element.element_type(), element.input_count(),
                                     element.output_count())) [[unlikely]] {
        throw_exception("element has wrong input or output count.");
    }
}

auto validate_connection_data(const connection_t connection_data) -> void {
    if (connection_data.element_id != null_element
        && connection_data.connection_id == null_connection) [[unlikely]] {
        throw_exception("Connection to an element cannot have null_connection.");
    }

    if (connection_data.element_id == null_element
        && connection_data.connection_id != null_connection) [[unlikely]] {
        throw_exception("Connection with null_element requires null_connection.");
    }
}

auto validate_sub_circuit_ids(const Schematic::ConstElement element) -> void {
    if (!(element.is_sub_circuit() == bool {element.sub_circuit_id()})) [[unlikely]] {
        throw_exception("Not a sub-circuit or no circuit id.");
    }
}

auto Schematic::validate(ValidationSettings settings) const -> void {
    // connections
    std::ranges::for_each(elements(), validate_input_output_count);
    for (const auto &data : input_connections_) {
        std::ranges::for_each(data, validate_connection_data);
    }
    for (const auto &data : output_connections_) {
        std::ranges::for_each(data, validate_connection_data);
    }

    std::ranges::for_each(elements(), validate_element_connections_consistent);
    std::ranges::for_each(elements(), validate_element_connections_no_loops);

    if (settings.require_all_outputs_connected) {
        std::ranges::for_each(elements(), validate_all_outputs_connected);
    }

    if (settings.require_all_placeholders_connected) {
        std::ranges::for_each(elements(), validate_placeholder_connected);
    }

    // simulation attributes
    std::ranges::for_each(elements(), validate_sub_circuit_ids);

    // TODO check new data members
    // * input_inverters_
    // * output_delays_
    // * history_lengths_

    // global attributes
    if (!circuit_id_) [[unlikely]] {
        throw_exception("invalid circuit id");
    }
    const auto input_count = accumulate(
        transform_view(elements(), &ConstElement::input_count), std::size_t {0});
    const auto output_count = accumulate(
        transform_view(elements(), &ConstElement::output_count), std::size_t {0});

    if (input_count != input_count_) [[unlikely]] {
        throw_exception("input count is wrong");
    }
    if (output_count != output_count_) [[unlikely]] {
        throw_exception("input count is wrong");
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
auto Schematic::ElementTemplate<Const>::schematic() const noexcept -> SchematicType & {
    return *schematic_;
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::element_id() const noexcept -> element_id_t {
    return element_id_;
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::element_type() const -> ElementType {
    return schematic_->element_types_.at(element_id_.value);
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::is_unused() const -> bool {
    return element_type() == ElementType::unused;
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::is_placeholder() const -> bool {
    return element_type() == ElementType::placeholder;
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::is_wire() const -> bool {
    return element_type() == ElementType::wire;
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::is_logic_item() const -> bool {
    return ::logicsim::is_logic_item(element_type());
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::is_sub_circuit() const -> bool {
    return element_type() == ElementType::sub_circuit;
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::sub_circuit_id() const -> circuit_id_t {
    return schematic_->sub_circuit_ids_.at(element_id_.value);
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::input_inverters() const
    -> const logic_small_vector_t & {
    return schematic_->input_inverters_.at(element_id_.value);
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::output_delays() const -> const output_delays_t & {
    return schematic_->output_delays_.at(element_id_.value);
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::history_length() const -> delay_t {
    return schematic_->history_lengths_.at(element_id_.value);
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::input_count() const -> std::size_t {
    return schematic_->input_connections_.at(element_id_.value).size();
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::output_count() const -> std::size_t {
    return schematic_->output_connections_.at(element_id_.value).size();
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
void Schematic::ElementTemplate<Const>::clear_all_connection() const
    requires(!Const)
{
    std::ranges::for_each(inputs(), &Input::clear_connection);
    std::ranges::for_each(outputs(), &Output::clear_connection);
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::set_history_length(delay_t value) const -> void
    requires(!Const)
{
    schematic_->history_lengths_.at(element_id_.value) = value;
}

template <bool Const>
auto Schematic::ElementTemplate<Const>::set_output_delays(
    std::vector<delay_t> delays) const -> void
    requires(!Const)
{
    if (std::size(delays) != output_count()) [[unlikely]] {
        throw_exception("Need as many delays as outputs.");
    }
    schematic_->output_delays_.at(element_id_.value)
        = output_delays_t {std::begin(delays), std::end(delays)};
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
Schematic::InputTemplate<Const>::operator connection_t() const noexcept {
    return {element_id(), input_index()};
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
auto Schematic::InputTemplate<Const>::schematic() const noexcept -> SchematicType & {
    return *schematic_;
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
    return connection_data_().connection_id;
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
            schematic_->output_connections_.at(connection_data.element_id.value)
                .at(connection_data.connection_id.value)};

        destination_connection_data.element_id = null_element;
        destination_connection_data.connection_id = null_connection;

        connection_data.element_id = null_element;
        connection_data.connection_id = null_connection;
    }
}

template <bool Const>
template <bool ConstOther>
void Schematic::InputTemplate<Const>::connect(OutputTemplate<ConstOther> output) const
    requires(!Const)
{
    clear_connection();
    schematic_->output(output).clear_connection();
    assert(!has_connected_element());
    assert(!output.has_connected_element());

    // get data before we modify anything, for exception safety
    auto &destination_connection_data
        = schematic_->output_connections_.at(output.element_id().value)
              .at(output.output_index().value);

    auto &connection_data {connection_data_()};

    connection_data.element_id = output.element_id();
    connection_data.connection_id = output.output_index();

    destination_connection_data.element_id = element_id();
    destination_connection_data.connection_id = input_index();
}

template <bool Const>
auto Schematic::InputTemplate<Const>::connection_data_() const -> ConnectionDataType & {
    return schematic_->input_connections_.at(element_id_.value).at(input_index_.value);
}

template <bool Const>
auto Schematic::InputTemplate<Const>::is_inverted() const -> bool {
    return schematic_->input_inverters_.at(element_id_.value).at(input_index_.value);
}

template <bool Const>
void Schematic::InputTemplate<Const>::set_inverted(bool value) const
    requires(!Const)
{
    schematic_->input_inverters_.at(element_id_.value).at(input_index_.value) = value;
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
Schematic::OutputTemplate<Const>::operator connection_t() const noexcept {
    return {element_id(), output_index()};
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
auto Schematic::OutputTemplate<Const>::schematic() const noexcept -> SchematicType & {
    return *schematic_;
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
    return connection_data_().connection_id;
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
            = schematic_->input_connections_.at(connection_data.element_id.value)
                  .at(connection_data.connection_id.value);

        destination_connection_data.element_id = null_element;
        destination_connection_data.connection_id = null_connection;

        connection_data.element_id = null_element;
        connection_data.connection_id = null_connection;
    }
}

template <bool Const>
template <bool ConstOther>
void Schematic::OutputTemplate<Const>::connect(InputTemplate<ConstOther> input) const
    requires(!Const)
{
    clear_connection();
    schematic_->input(input).clear_connection();
    assert(!has_connected_element());
    assert(!input.has_connected_element());

    // get data before we modify anything, for exception safety
    auto &connection_data {connection_data_()};
    auto &destination_connection_data
        = schematic_->input_connections_.at(input.element_id().value)
              .at(input.input_index().value);

    connection_data.element_id = input.element_id();
    connection_data.connection_id = input.input_index();

    destination_connection_data.element_id = element_id();
    destination_connection_data.connection_id = output_index();
}

template <bool Const>
auto Schematic::OutputTemplate<Const>::delay() const -> const delay_t {
    return schematic_->output_delays_.at(element_id_.value).at(output_index_.value);
}

template <bool Const>
auto Schematic::OutputTemplate<Const>::set_delay(delay_t value) const -> void
    requires(!Const)
{
    schematic_->output_delays_.at(element_id_.value).at(output_index_.value) = value;
}

template <bool Const>
auto Schematic::OutputTemplate<Const>::connection_data_() const -> ConnectionDataType & {
    return schematic_->output_connections_.at(element_id_.value).at(output_index_.value);
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
        auto placeholder = output.schematic().add_element(Schematic::ElementData {
            .element_type = ElementType::placeholder,
            .input_count = 1,
            .output_count = 0,
        });
        output.connect(placeholder.input(connection_id_t {0}));
    }
}

auto add_element_placeholders(Schematic::Element element) -> void {
    std::ranges::for_each(element.outputs(), add_placeholder);
}

auto calculate_output_delays(const LineTree &line_tree, delay_t wire_delay_per_distance)
    -> std::vector<delay_t> {
    auto lengths = line_tree.calculate_output_lengths();
    return transform_to_vector(lengths, [&](LineTree::length_t length) -> delay_t {
        // TODO handle overflow
        return delay_t {wire_delay_per_distance.value * length};
    });
}

auto add_output_placeholders(Schematic &schematic) -> void {
    std::ranges::for_each(schematic.elements(), add_element_placeholders);
}

auto benchmark_schematic(const int n_elements) -> Schematic {
    Schematic schematic {};

    auto elem0 = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::and_element,
        .input_count = 2,
        .output_count = 1,
    });

    for ([[maybe_unused]] auto count : range(n_elements - 1)) {
        auto wire0 = schematic.add_element(Schematic::ElementData {
            .element_type = ElementType::wire,
            .input_count = 1,
            .output_count = 2,
        });
        auto elem1 = schematic.add_element(Schematic::ElementData {
            .element_type = ElementType::and_element,
            .input_count = 2,
            .output_count = 1,
        });

        elem0.output(connection_id_t {0}).connect(wire0.input(connection_id_t {0}));

        wire0.output(connection_id_t {0}).connect(elem1.input(connection_id_t {0}));
        wire0.output(connection_id_t {1}).connect(elem1.input(connection_id_t {1}));

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

    const auto element_type = element_dist(rng) == 0
                                  ? ElementType::xor_element
                                  : (element_dist(rng) == 1 ? ElementType::buffer_element
                                                            : ElementType::wire);

    const auto input_count
        = element_type == ElementType::xor_element ? connection_dist(rng) : 1;

    const auto output_count
        = element_type == ElementType::wire ? connection_dist(rng) : 1;

    schematic.add_element(Schematic::ElementData {
        .element_type = element_type,
        .input_count = gsl::narrow<std::size_t>(input_count),
        .output_count = gsl::narrow<std::size_t>(output_count),
        .input_inverters = element_type == ElementType::buffer_element
                               ? logic_small_vector_t {true}
                               : logic_small_vector_t {},
    });
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
