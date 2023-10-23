
#include "schematic_old.h"

#include "algorithm/accumulate.h"
#include "algorithm/fmt_join.h"
#include "algorithm/range.h"
#include "algorithm/transform_to_vector.h"
#include "exception.h"
#include "format/container.h"
#include "iterator_adaptor/transform_view.h"
#include "layout_info.h"
#include "line_tree.h"

#include <fmt/core.h>
#include <gsl/gsl>

#include <algorithm>
#include <utility>

namespace logicsim {

//
// SchematicOld
//

SchematicOld::SchematicOld(circuit_id_t circuit_id) : circuit_id_ {circuit_id} {}

SchematicOld::SchematicOld(delay_t wire_delay_per_distance)
    : wire_delay_per_distance_ {wire_delay_per_distance} {}

SchematicOld::SchematicOld(circuit_id_t circuit_id, delay_t wire_delay_per_distance)
    : circuit_id_ {circuit_id}, wire_delay_per_distance_ {wire_delay_per_distance} {}

auto SchematicOld::clear() -> void {
    input_connections_.clear();
    output_connections_.clear();
    sub_circuit_ids_.clear();
    element_types_.clear();
    input_inverters_.clear();
    output_delays_.clear();
    history_lengths_.clear();

    total_input_count_ = 0;
    total_output_count_ = 0;
}

auto SchematicOld::swap(SchematicOld &other) noexcept -> void {
    using std::swap;

    input_connections_.swap(other.input_connections_);
    output_connections_.swap(other.output_connections_);
    sub_circuit_ids_.swap(other.sub_circuit_ids_);
    element_types_.swap(other.element_types_);
    input_inverters_.swap(other.input_inverters_);
    output_delays_.swap(other.output_delays_);
    history_lengths_.swap(other.history_lengths_);

    swap(total_input_count_, other.total_input_count_);
    swap(total_output_count_, other.total_output_count_);
    swap(circuit_id_, other.circuit_id_);
}

auto SchematicOld::swap_element_data(element_id_t element_id_1, element_id_t element_id_2,
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

auto SchematicOld::delete_last_element(bool clear_connections) -> void {
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
    if (total_input_count_ < last_input_count || total_output_count_ < last_output_count)
        [[unlikely]] {
        throw_exception("input or output count underflows");
    }
    total_input_count_ -= last_input_count;
    total_output_count_ -= last_output_count;

    // delete
    input_connections_.pop_back();
    output_connections_.pop_back();
    sub_circuit_ids_.pop_back();
    element_types_.pop_back();
    input_inverters_.pop_back();
    output_delays_.pop_back();
    history_lengths_.pop_back();
}

auto swap(SchematicOld &a, SchematicOld &b) noexcept -> void {
    a.swap(b);
}

}  // namespace logicsim

template <>
auto std::swap(logicsim::SchematicOld &a, logicsim::SchematicOld &b) noexcept -> void {
    a.swap(b);
}

namespace logicsim {

auto SchematicOld::format() const -> std::string {
    std::string inner {};
    if (!empty()) {
        auto format_true = [](auto element) { return element.format(true); };
        inner =
            fmt::format(": [\n  {}\n]", fmt_join(",\n  ", elements(), "{}", format_true));
    }
    return fmt::format("<SchematicOld with {} elements{}>", element_count(), inner);
}

auto SchematicOld::circuit_id() const noexcept -> circuit_id_t {
    return circuit_id_;
}

auto SchematicOld::element_count() const noexcept -> std::size_t {
    return element_types_.size();
}

auto SchematicOld::empty() const noexcept -> bool {
    return element_types_.empty();
}

auto SchematicOld::is_element_id_valid(element_id_t element_id) const noexcept -> bool {
    auto size = gsl::narrow_cast<element_id_t::value_type>(element_count());
    return element_id >= element_id_t {0} && element_id < element_id_t {size};
}

auto SchematicOld::element_ids() const noexcept -> forward_range_t<element_id_t> {
    const auto count =
        element_id_t {gsl::narrow_cast<element_id_t::value_type>(element_count())};
    return range(count);
}

auto SchematicOld::element(element_id_t element_id) -> Element {
    if (!is_element_id_valid(element_id)) [[unlikely]] {
        throw_exception("Element id is invalid");
    }
    return Element {*this, element_id};
}

auto SchematicOld::element(element_id_t element_id) const -> ConstElement {
    if (!is_element_id_valid(element_id)) [[unlikely]] {
        throw_exception("Element id is invalid");
    }
    return ConstElement {*this, element_id};
}

auto SchematicOld::elements() noexcept -> ElementView {
    return ElementView {*this};
}

auto SchematicOld::elements() const noexcept -> ConstElementView {
    return ConstElementView {*this};
}

auto SchematicOld::input(connection_t connection) -> Input {
    return element(connection.element_id).input(connection.connection_id);
}

auto SchematicOld::input(connection_t connection) const -> ConstInput {
    return element(connection.element_id).input(connection.connection_id);
}

auto SchematicOld::output(connection_t connection) -> Output {
    return element(connection.element_id).output(connection.connection_id);
}

auto SchematicOld::output(connection_t connection) const -> ConstOutput {
    return element(connection.element_id).output(connection.connection_id);
}

auto SchematicOld::wire_delay_per_distance() const -> delay_t {
    return wire_delay_per_distance_;
}

auto SchematicOld::add_element(ElementData &&data) -> Element {
    // make sure we can represent all ids
    if (element_types_.size() >= std::size_t {element_id_t::max()} - std::size_t {1})
        [[unlikely]] {
        throw_exception("Reached maximum number of elements.");
    }
    if (total_input_count_ >= std::numeric_limits<std::size_t>::max() -
                                  std::size_t {data.input_count.count()}) [[unlikely]] {
        throw_exception("Reached maximum number of inputs.");
    }
    if (total_output_count_ >= std::numeric_limits<std::size_t>::max() -
                                   std::size_t {data.output_count.count()}) [[unlikely]] {
        throw_exception("Reached maximum number of outputs.");
    }

    // extend vectors
    element_types_.push_back(data.element_type);
    sub_circuit_ids_.push_back(data.sub_circuit_id);
    input_connections_.emplace_back(data.input_count.count(),
                                    connection_t {null_element, null_connection_id});
    output_connections_.emplace_back(data.output_count.count(),
                                     connection_t {null_element, null_connection_id});
    if (data.input_inverters.size() == 0) {
        input_inverters_.emplace_back(data.input_count.count(), false);
    } else {
        if (data.input_inverters.size() != std::size_t {data.input_count}) [[unlikely]] {
            throw_exception("Need as many values for input_inverters as inputs.");
        }
        input_inverters_.emplace_back(data.input_inverters);
    }
    {
        if (data.output_delays.size() != std::size_t {data.output_count}) [[unlikely]] {
            throw_exception("Need as many output_delays as outputs.");
        }
        output_delays_.emplace_back(std::begin(data.output_delays),
                                    std::end(data.output_delays));
    }
    history_lengths_.push_back(data.history_length);

    // return element
    auto element_id = element_id_t {gsl::narrow_cast<element_id_t::value_type>(
        element_types_.size() - std::size_t {1})};

    total_input_count_ += std::size_t {data.input_count};
    total_output_count_ += std::size_t {data.output_count};

    return element(element_id);
}

auto SchematicOld::swap_and_delete_element(element_id_t element_id) -> element_id_t {
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

auto SchematicOld::swap_elements(element_id_t element_id_0, element_id_t element_id_1)
    -> void {
    swap_element_data(element_id_0, element_id_1, true);
}

auto SchematicOld::update_swapped_connections(element_id_t new_element_id,
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

auto SchematicOld::total_input_count() const noexcept -> std::size_t {
    return total_input_count_;
}

auto SchematicOld::total_output_count() const noexcept -> std::size_t {
    return total_output_count_;
}

//
// Element Iterator
//

template <bool Const>
SchematicOld::ElementIteratorTemplate<Const>::ElementIteratorTemplate(
    schematic_type &schematic, element_id_t element_id) noexcept
    : schematic_(&schematic), element_id_(element_id) {}

template <bool Const>
auto SchematicOld::ElementIteratorTemplate<Const>::operator*() const -> value_type {
    if (schematic_ == nullptr) [[unlikely]] {
        throw_exception("schematic cannot be null when dereferencing element iterator");
    }
    return schematic_->element(element_id_);
}

template <bool Const>
auto SchematicOld::ElementIteratorTemplate<Const>::operator++() noexcept
    -> SchematicOld::ElementIteratorTemplate<Const> & {
    ++element_id_;
    return *this;
}

template <bool Const>
auto SchematicOld::ElementIteratorTemplate<Const>::operator++(int) noexcept
    -> SchematicOld::ElementIteratorTemplate<Const> {
    auto tmp = *this;
    ++(*this);
    return tmp;
}

template <bool Const>
auto SchematicOld::ElementIteratorTemplate<Const>::operator==(
    const ElementIteratorTemplate &right) const noexcept -> bool {
    return element_id_ >= right.element_id_;
}

template <bool Const>
auto SchematicOld::ElementIteratorTemplate<Const>::operator-(
    const ElementIteratorTemplate &right) const -> difference_type {
    if (!element_id_ || !right.element_id_) [[unlikely]] {
        throw std::runtime_error("element ids need to be valid");
    }
    return difference_type {element_id_.value} -
           difference_type {right.element_id_.value};
}

template class SchematicOld::ElementIteratorTemplate<false>;
template class SchematicOld::ElementIteratorTemplate<true>;

//
// Element View
//

template <bool Const>
SchematicOld::ElementViewTemplate<Const>::ElementViewTemplate(
    schematic_type &schematic) noexcept
    : schematic_(&schematic) {}

template <bool Const>
auto SchematicOld::ElementViewTemplate<Const>::begin() const noexcept -> iterator_type {
    return iterator_type {*schematic_, element_id_t {0}};
}

template <bool Const>
auto SchematicOld::ElementViewTemplate<Const>::end() const noexcept -> iterator_type {
    return iterator_type {*schematic_,
                          element_id_t {gsl::narrow_cast<element_id_t::value_type>(
                              schematic_->element_count())}};
}

template <bool Const>
auto SchematicOld::ElementViewTemplate<Const>::size() const noexcept -> std::size_t {
    return schematic_->element_count();
}

template <bool Const>
auto SchematicOld::ElementViewTemplate<Const>::empty() const noexcept -> bool {
    return schematic_->empty();
}

template class SchematicOld::ElementViewTemplate<false>;
template class SchematicOld::ElementViewTemplate<true>;

static_assert(std::ranges::input_range<SchematicOld::ElementView>);
static_assert(std::ranges::input_range<SchematicOld::ConstElementView>);

//
// SchematicOld::Element
//

template <bool Const>
SchematicOld::ElementTemplate<Const>::ElementTemplate(SchematicType &schematic,
                                                      element_id_t element_id) noexcept
    : schematic_(&schematic), element_id_(element_id) {}

template <bool Const>
template <bool ConstOther>
SchematicOld::ElementTemplate<Const>::ElementTemplate(
    ElementTemplate<ConstOther> element) noexcept
    requires Const && (!ConstOther)
    : schematic_(element.schematic_), element_id_(element.element_id_) {}

template <bool Const>
SchematicOld::ElementTemplate<Const>::operator element_id_t() const noexcept {
    return element_id_;
}

template <bool Const>
template <bool ConstOther>
auto SchematicOld::ElementTemplate<Const>::operator==(
    ElementTemplate<ConstOther> other) const noexcept -> bool {
    return schematic_ == other.schematic_ && element_id_ == other.element_id_;
}

template <bool Const>
auto SchematicOld::ElementTemplate<Const>::format(bool with_connections) const
    -> std::string {
    auto connections = !with_connections
                           ? ""
                           : fmt::format(", inputs = {}, outputs = {}", inputs().format(),
                                         outputs().format());

    return fmt::format("<Element {}: {}x{} {}{}>", element_id(), input_count(),
                       output_count(), element_type(), connections);
}

template <bool Const>
auto SchematicOld::ElementTemplate<Const>::schematic() const noexcept -> SchematicType & {
    return *schematic_;
}

template <bool Const>
auto SchematicOld::ElementTemplate<Const>::element_id() const noexcept -> element_id_t {
    return element_id_;
}

template <bool Const>
auto SchematicOld::ElementTemplate<Const>::element_type() const -> ElementType {
    return schematic_->element_types_.at(element_id_.value);
}

template <bool Const>
auto SchematicOld::ElementTemplate<Const>::is_unused() const -> bool {
    return element_type() == ElementType::unused;
}

template <bool Const>
auto SchematicOld::ElementTemplate<Const>::is_placeholder() const -> bool {
    return element_type() == ElementType::placeholder;
}

template <bool Const>
auto SchematicOld::ElementTemplate<Const>::is_wire() const -> bool {
    return element_type() == ElementType::wire;
}

template <bool Const>
auto SchematicOld::ElementTemplate<Const>::is_logic_item() const -> bool {
    return ::logicsim::is_logic_item(element_type());
}

template <bool Const>
auto SchematicOld::ElementTemplate<Const>::is_sub_circuit() const -> bool {
    return element_type() == ElementType::sub_circuit;
}

template <bool Const>
auto SchematicOld::ElementTemplate<Const>::sub_circuit_id() const -> circuit_id_t {
    return schematic_->sub_circuit_ids_.at(element_id_.value);
}

template <bool Const>
auto SchematicOld::ElementTemplate<Const>::input_inverters() const
    -> const logic_small_vector_t & {
    return schematic_->input_inverters_.at(element_id_.value);
}

template <bool Const>
auto SchematicOld::ElementTemplate<Const>::output_delays() const
    -> const output_delays_t & {
    return schematic_->output_delays_.at(element_id_.value);
}

template <bool Const>
auto SchematicOld::ElementTemplate<Const>::history_length() const -> delay_t {
    return schematic_->history_lengths_.at(element_id_.value);
}

template <bool Const>
auto SchematicOld::ElementTemplate<Const>::input_count() const -> connection_count_t {
    return connection_count_t {
        schematic_->input_connections_.at(element_id_.value).size()};
}

template <bool Const>
auto SchematicOld::ElementTemplate<Const>::output_count() const -> connection_count_t {
    return connection_count_t {
        schematic_->output_connections_.at(element_id_.value).size()};
}

template <bool Const>
auto SchematicOld::ElementTemplate<Const>::input(connection_id_t input) const
    -> InputTemplate<Const> {
    return InputTemplate<Const> {*schematic_, element_id_, input};
}

template <bool Const>
auto SchematicOld::ElementTemplate<Const>::output(connection_id_t output) const
    -> OutputTemplate<Const> {
    return OutputTemplate<Const> {*schematic_, element_id_, output};
}

template <bool Const>
inline auto SchematicOld::ElementTemplate<Const>::inputs() const
    -> InputViewTemplate<Const> {
    return InputViewTemplate<Const> {*this};
}

template <bool Const>
inline auto SchematicOld::ElementTemplate<Const>::outputs() const
    -> OutputViewTemplate<Const> {
    return OutputViewTemplate<Const> {*this};
}

template <bool Const>
void SchematicOld::ElementTemplate<Const>::clear_all_connection() const
    requires(!Const)
{
    std::ranges::for_each(inputs(), &Input::clear_connection);
    std::ranges::for_each(outputs(), &Output::clear_connection);
}

template <bool Const>
auto SchematicOld::ElementTemplate<Const>::set_history_length(delay_t value) const -> void
    requires(!Const)
{
    schematic_->history_lengths_.at(element_id_.value) = value;
}

template <bool Const>
auto SchematicOld::ElementTemplate<Const>::set_output_delays(
    std::vector<delay_t> delays) const -> void
    requires(!Const)
{
    if (connection_count_t {std::size(delays)} != output_count()) [[unlikely]] {
        throw_exception("Need as many delays as outputs.");
    }
    schematic_->output_delays_.at(element_id_.value) =
        output_delays_t {std::begin(delays), std::end(delays)};
}

// Template Instanciations

template class SchematicOld::ElementTemplate<true>;
template class SchematicOld::ElementTemplate<false>;

template SchematicOld::ElementTemplate<true>::ElementTemplate(
    ElementTemplate<false>) noexcept;

template auto SchematicOld::ElementTemplate<false>::operator==
    <false>(ElementTemplate<false>) const noexcept -> bool;
template auto SchematicOld::ElementTemplate<false>::operator==
    <true>(ElementTemplate<true>) const noexcept -> bool;
template auto SchematicOld::ElementTemplate<true>::operator==
    <false>(ElementTemplate<false>) const noexcept -> bool;
template auto SchematicOld::ElementTemplate<true>::operator==
    <true>(ElementTemplate<true>) const noexcept -> bool;

//
// Connection Iterator
//

template <bool Const, bool IsInput>
auto SchematicOld::ConnectionIteratorTemplate<Const, IsInput>::operator*() const
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
auto SchematicOld::ConnectionIteratorTemplate<Const, IsInput>::operator++() noexcept
    -> SchematicOld::ConnectionIteratorTemplate<Const, IsInput> & {
    ++connection_id;
    return *this;
}

template <bool Const, bool IsInput>
auto SchematicOld::ConnectionIteratorTemplate<Const, IsInput>::operator++(int) noexcept
    -> ConnectionIteratorTemplate {
    auto tmp = *this;
    ++(*this);
    return tmp;
}

template <bool Const, bool IsInput>
auto SchematicOld::ConnectionIteratorTemplate<Const, IsInput>::operator==(
    const ConnectionIteratorTemplate &right) const noexcept -> bool {
    // TODO why not equal?
    return connection_id >= right.connection_id;
}

template <bool Const, bool IsInput>
auto SchematicOld::ConnectionIteratorTemplate<Const, IsInput>::operator-(
    const ConnectionIteratorTemplate &right) const -> difference_type {
    if (!connection_id || !right.connection_id) [[unlikely]] {
        throw std::runtime_error("connection ids need to be valid");
    }
    return difference_type {connection_id.value} -
           difference_type {right.connection_id.value};
}

template class SchematicOld::ConnectionIteratorTemplate<false, false>;
template class SchematicOld::ConnectionIteratorTemplate<true, false>;
template class SchematicOld::ConnectionIteratorTemplate<false, true>;
template class SchematicOld::ConnectionIteratorTemplate<true, true>;

//
// Connection View
//

template <bool Const, bool IsInput>
SchematicOld::ConnectionViewTemplate<Const, IsInput>::ConnectionViewTemplate(
    ElementTemplate<Const> element) noexcept
    : element_(element) {}

template <bool Const, bool IsInput>
auto SchematicOld::ConnectionViewTemplate<Const, IsInput>::begin() const
    -> iterator_type {
    return iterator_type {element_, connection_id_t {0}};
}

template <bool Const, bool IsInput>
auto SchematicOld::ConnectionViewTemplate<Const, IsInput>::end() const -> iterator_type {
    return iterator_type {
        element_,
        connection_id_t {gsl::narrow_cast<connection_id_t::value_type>(size())}};
}

template <bool Const, bool IsInput>
auto SchematicOld::ConnectionViewTemplate<Const, IsInput>::size() const -> std::size_t {
    if constexpr (IsInput) {
        return std::size_t {element_.input_count()};
    } else {
        return std::size_t {element_.output_count()};
    }
}

template <bool Const, bool IsInput>
auto SchematicOld::ConnectionViewTemplate<Const, IsInput>::empty() const -> bool {
    return size() == std::size_t {0};
}

template <bool Const, bool IsInput>
auto SchematicOld::ConnectionViewTemplate<Const, IsInput>::format() const -> std::string {
    auto connections = transform_view(*this, &value_type::format_connection);
    return fmt::format("{}", connections);
}

template class SchematicOld::ConnectionViewTemplate<false, false>;
template class SchematicOld::ConnectionViewTemplate<true, false>;
template class SchematicOld::ConnectionViewTemplate<false, true>;
template class SchematicOld::ConnectionViewTemplate<true, true>;

static_assert(std::ranges::input_range<SchematicOld::InputView>);
static_assert(std::ranges::input_range<SchematicOld::ConstInputView>);
static_assert(std::ranges::input_range<SchematicOld::OutputView>);
static_assert(std::ranges::input_range<SchematicOld::ConstOutputView>);

//
// SchematicOld::Input
//

template <bool Const>
SchematicOld::InputTemplate<Const>::InputTemplate(SchematicType &schematic,
                                                  element_id_t element_id,
                                                  connection_id_t input_index) noexcept
    : schematic_(&schematic), element_id_(element_id), input_index_(input_index) {}

template <bool Const>
template <bool ConstOther>
SchematicOld::InputTemplate<Const>::InputTemplate(
    InputTemplate<ConstOther> input) noexcept
    requires Const && (!ConstOther)
    : schematic_(input.schematic_),
      element_id_(input.element_id_),
      input_index_(input.input_index_) {}

template <bool Const>
template <bool ConstOther>
auto SchematicOld::InputTemplate<Const>::operator==(
    InputTemplate<ConstOther> other) const noexcept -> bool {
    return schematic_ == other.schematic_ && element_id_ == other.element_id_ &&
           input_index_ == other.input_index_;
}

template <bool Const>
SchematicOld::InputTemplate<Const>::operator connection_t() const noexcept {
    return connection_t {element_id(), input_index()};
}

template <bool Const>
auto SchematicOld::InputTemplate<Const>::format() const -> std::string {
    const auto element = this->element();
    return fmt::format("<Input {} of Element {}: {} {} x {}>", input_index(),
                       element_id(), element.element_type(), element.input_count(),
                       element.output_count());
}

template <bool Const>
auto SchematicOld::InputTemplate<Const>::format_connection() const -> std::string {
    if (has_connected_element()) {
        return fmt::format("Element_{}-{}", connected_element_id(),
                           connected_output_index());
    }
    return "---";
}

template <bool Const>
auto SchematicOld::InputTemplate<Const>::schematic() const noexcept -> SchematicType & {
    return *schematic_;
}

template <bool Const>
auto SchematicOld::InputTemplate<Const>::element_id() const noexcept -> element_id_t {
    return element_id_;
}

template <bool Const>
auto SchematicOld::InputTemplate<Const>::input_index() const noexcept -> connection_id_t {
    return input_index_;
}

template <bool Const>
auto SchematicOld::InputTemplate<Const>::element() const -> ElementTemplate<Const> {
    return schematic_->element(element_id_);
}

template <bool Const>
auto SchematicOld::InputTemplate<Const>::has_connected_element() const -> bool {
    return connected_element_id() != null_element;
}

template <bool Const>
auto SchematicOld::InputTemplate<Const>::connected_element_id() const -> element_id_t {
    return connection_data_().element_id;
}

template <bool Const>
auto SchematicOld::InputTemplate<Const>::connected_output_index() const
    -> connection_id_t {
    return connection_data_().connection_id;
}

template <bool Const>
auto SchematicOld::InputTemplate<Const>::connected_element() const
    -> ElementTemplate<Const> {
    return schematic_->element(connected_element_id());
}

template <bool Const>
auto SchematicOld::InputTemplate<Const>::connected_output() const
    -> OutputTemplate<Const> {
    return connected_element().output(connected_output_index());
}

template <bool Const>
void SchematicOld::InputTemplate<Const>::clear_connection() const
    requires(!Const)
{
    auto &connection_data {connection_data_()};
    if (connection_data.element_id != null_element) {
        auto &destination_connection_data {
            schematic_->output_connections_.at(connection_data.element_id.value)
                .at(connection_data.connection_id.value)};

        destination_connection_data.element_id = null_element;
        destination_connection_data.connection_id = null_connection_id;

        connection_data.element_id = null_element;
        connection_data.connection_id = null_connection_id;
    }
}

template <bool Const>
template <bool ConstOther>
void SchematicOld::InputTemplate<Const>::connect(OutputTemplate<ConstOther> output) const
    requires(!Const)
{
    clear_connection();
    schematic_->output(output).clear_connection();
    assert(!has_connected_element());
    assert(!output.has_connected_element());

    // get data before we modify anything, for exception safety
    auto &destination_connection_data =
        schematic_->output_connections_.at(output.element_id().value)
            .at(output.output_index().value);

    auto &connection_data {connection_data_()};

    connection_data.element_id = output.element_id();
    connection_data.connection_id = output.output_index();

    destination_connection_data.element_id = element_id();
    destination_connection_data.connection_id = input_index();
}

template <bool Const>
auto SchematicOld::InputTemplate<Const>::connection_data_() const
    -> ConnectionDataType & {
    return schematic_->input_connections_.at(element_id_.value).at(input_index_.value);
}

template <bool Const>
auto SchematicOld::InputTemplate<Const>::is_inverted() const -> bool {
    return schematic_->input_inverters_.at(element_id_.value).at(input_index_.value);
}

template <bool Const>
void SchematicOld::InputTemplate<Const>::set_inverted(bool value) const
    requires(!Const)
{
    schematic_->input_inverters_.at(element_id_.value).at(input_index_.value) = value;
}

// Template Instanciations

template class SchematicOld::InputTemplate<true>;
template class SchematicOld::InputTemplate<false>;

template SchematicOld::InputTemplate<true>::InputTemplate(InputTemplate<false>) noexcept;

template auto SchematicOld::InputTemplate<false>::operator==
    <false>(InputTemplate<false>) const noexcept -> bool;
template auto SchematicOld::InputTemplate<false>::operator==
    <true>(InputTemplate<true>) const noexcept -> bool;
template auto SchematicOld::InputTemplate<true>::operator==
    <false>(InputTemplate<false>) const noexcept -> bool;
template auto SchematicOld::InputTemplate<true>::operator==
    <true>(InputTemplate<true>) const noexcept -> bool;

template void SchematicOld::InputTemplate<false>::connect<false>(
    OutputTemplate<false>) const;
template void SchematicOld::InputTemplate<false>::connect<true>(
    OutputTemplate<true>) const;

//
// SchematicOld::Output
//

template <bool Const>
SchematicOld::OutputTemplate<Const>::OutputTemplate(SchematicType &schematic,
                                                    element_id_t element_id,
                                                    connection_id_t output_index) noexcept
    : schematic_(&schematic), element_id_(element_id), output_index_(output_index) {}

template <bool Const>
template <bool ConstOther>
SchematicOld::OutputTemplate<Const>::OutputTemplate(
    OutputTemplate<ConstOther> output) noexcept
    requires Const && (!ConstOther)
    : schematic_(output.schematic_),
      element_id_(output.element_id_),
      output_index_(output.output_index_) {}

template <bool Const>
template <bool ConstOther>
auto SchematicOld::OutputTemplate<Const>::operator==(
    SchematicOld::OutputTemplate<ConstOther> other) const noexcept -> bool {
    return schematic_ == other.schematic_ && element_id_ == other.element_id_ &&
           output_index_ == other.output_index_;
}

template <bool Const>
SchematicOld::OutputTemplate<Const>::operator connection_t() const noexcept {
    return connection_t {element_id(), output_index()};
}

template <bool Const>
auto SchematicOld::OutputTemplate<Const>::format() const -> std::string {
    const auto element = this->element();
    return fmt::format("<Output {} of Element {}: {} {} x {}>", output_index(),
                       element_id(), element.element_type(), element.input_count(),
                       element.output_count());
}

template <bool Const>
auto SchematicOld::OutputTemplate<Const>::format_connection() const -> std::string {
    if (has_connected_element()) {
        return fmt::format("Element_{}-{}", connected_element_id(),
                           connected_input_index());
    }
    return "---";
}

template <bool Const>
auto SchematicOld::OutputTemplate<Const>::schematic() const noexcept -> SchematicType & {
    return *schematic_;
}

template <bool Const>
auto SchematicOld::OutputTemplate<Const>::element_id() const noexcept -> element_id_t {
    return element_id_;
}

template <bool Const>
auto SchematicOld::OutputTemplate<Const>::output_index() const noexcept
    -> connection_id_t {
    return output_index_;
}

template <bool Const>
auto SchematicOld::OutputTemplate<Const>::element() const -> ElementTemplate<Const> {
    return schematic_->element(element_id_);
}

template <bool Const>
auto SchematicOld::OutputTemplate<Const>::has_connected_element() const -> bool {
    return connected_element_id() != null_element;
}

template <bool Const>
auto SchematicOld::OutputTemplate<Const>::connected_element_id() const -> element_id_t {
    return connection_data_().element_id;
}

template <bool Const>
auto SchematicOld::OutputTemplate<Const>::connected_input_index() const
    -> connection_id_t {
    return connection_data_().connection_id;
}

template <bool Const>
auto SchematicOld::OutputTemplate<Const>::connected_element() const
    -> ElementTemplate<Const> {
    return schematic_->element(connected_element_id());
}

template <bool Const>
auto SchematicOld::OutputTemplate<Const>::connected_input() const
    -> InputTemplate<Const> {
    return connected_element().input(connected_input_index());
}

template <bool Const>
void SchematicOld::OutputTemplate<Const>::clear_connection() const
    requires(!Const)
{
    auto &connection_data {connection_data_()};
    if (connection_data.element_id != null_element) {
        auto &destination_connection_data =
            schematic_->input_connections_.at(connection_data.element_id.value)
                .at(connection_data.connection_id.value);

        destination_connection_data.element_id = null_element;
        destination_connection_data.connection_id = null_connection_id;

        connection_data.element_id = null_element;
        connection_data.connection_id = null_connection_id;
    }
}

template <bool Const>
template <bool ConstOther>
void SchematicOld::OutputTemplate<Const>::connect(InputTemplate<ConstOther> input) const
    requires(!Const)
{
    clear_connection();
    schematic_->input(input).clear_connection();
    assert(!has_connected_element());
    assert(!input.has_connected_element());

    // get data before we modify anything, for exception safety
    auto &connection_data {connection_data_()};
    auto &destination_connection_data =
        schematic_->input_connections_.at(input.element_id().value)
            .at(input.input_index().value);

    connection_data.element_id = input.element_id();
    connection_data.connection_id = input.input_index();

    destination_connection_data.element_id = element_id();
    destination_connection_data.connection_id = output_index();
}

template <bool Const>
auto SchematicOld::OutputTemplate<Const>::delay() const -> const delay_t {
    return schematic_->output_delays_.at(element_id_.value).at(output_index_.value);
}

template <bool Const>
auto SchematicOld::OutputTemplate<Const>::set_delay(delay_t value) const -> void
    requires(!Const)
{
    schematic_->output_delays_.at(element_id_.value).at(output_index_.value) = value;
}

template <bool Const>
auto SchematicOld::OutputTemplate<Const>::connection_data_() const
    -> ConnectionDataType & {
    return schematic_->output_connections_.at(element_id_.value).at(output_index_.value);
}

// Template Instanciations

template class SchematicOld::OutputTemplate<true>;
template class SchematicOld::OutputTemplate<false>;

template SchematicOld::OutputTemplate<true>::OutputTemplate(
    OutputTemplate<false>) noexcept;

template auto SchematicOld::OutputTemplate<false>::operator==
    <false>(OutputTemplate<false>) const noexcept -> bool;
template auto SchematicOld::OutputTemplate<false>::operator==
    <true>(OutputTemplate<true>) const noexcept -> bool;
template auto SchematicOld::OutputTemplate<true>::operator==
    <false>(OutputTemplate<false>) const noexcept -> bool;
template auto SchematicOld::OutputTemplate<true>::operator==
    <true>(OutputTemplate<true>) const noexcept -> bool;

template void SchematicOld::OutputTemplate<false>::connect<false>(
    InputTemplate<false>) const;
template void SchematicOld::OutputTemplate<false>::connect<true>(
    InputTemplate<true>) const;

}  // namespace logicsim

//
// Free Functions
//

namespace logicsim {

auto add_placeholder(SchematicOld::Output output) -> void {
    if (!output.has_connected_element()) {
        auto placeholder = output.schematic().add_element(SchematicOld::ElementData {
            .element_type = ElementType::placeholder,
            .input_count = connection_count_t {1},
            .output_count = connection_count_t {0},
        });
        output.connect(placeholder.input(connection_id_t {0}));
    }
}

auto add_element_placeholders(SchematicOld::Element element) -> void {
    std::ranges::for_each(element.outputs(), add_placeholder);
}

auto add_output_placeholders(SchematicOld &schematic) -> void {
    std::ranges::for_each(schematic.elements(), add_element_placeholders);
}

}  // namespace logicsim