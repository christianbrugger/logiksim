
#include "circuit.h"

#include <fmt/ranges.h>

#include <utility>

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
