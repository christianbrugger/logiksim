#include "editable_circuit.h"

#include "exceptions.h"
#include "layout_calculations.h"

#include <fmt/core.h>

#include <algorithm>

namespace logicsim {

EditableCircuit::EditableCircuit(Schematic&& schematic, Layout&& layout)
    : schematic_ {std::move(schematic)}, layout_ {std::move(layout)} {}

auto EditableCircuit::format() const -> std::string {
    return fmt::format("EditableCircuit{{\n{}\n{}\n}}", schematic_, layout_);
}

auto EditableCircuit::layout() const noexcept -> const Layout& {
    return layout_;
}

auto EditableCircuit::schematic() const noexcept -> const Schematic& {
    return schematic_;
}

auto EditableCircuit::copy_input_positions() -> std::vector<point_t> {
    return transform_to_vector(input_connections_,
                               [](auto value) { return value.first; });
}

auto EditableCircuit::copy_output_positions() -> std::vector<point_t> {
    return transform_to_vector(output_connections_,
                               [](auto value) { return value.first; });
}

auto EditableCircuit::add_placeholder_element() -> element_id_t {
    constexpr static auto connector_delay
        = delay_t {Schematic::defaults::wire_delay_per_distance.value / 2};

    const auto element_id = layout_.add_placeholder();
    {
        const auto element = schematic_.add_element(Schematic::NewElementData {
            .element_type = ElementType::placeholder,
            .input_count = 1,
            .output_count = 0,
            .history_length = connector_delay,
        });
        if (element.element_id() != element_id) [[unlikely]] {
            throw_exception("Added element ids don't match.");
        }
    }

    return element_id;
}

auto EditableCircuit::add_inverter_element(point_t position,
                                           DisplayOrientation orientation)
    -> element_id_t {
    return add_standard_element(ElementType::inverter_element, 1, position, orientation);
}

auto EditableCircuit::add_standard_element(ElementType type, std::size_t input_count,
                                           point_t position,
                                           DisplayOrientation orientation)
    -> element_id_t {
    using enum ElementType;
    if (!(type == and_element || type == or_element || type == xor_element
          || type == inverter_element)) [[unlikely]] {
        throw_exception("The type needs to be standard element.");
    }
    if (type == inverter_element && input_count != 1) {
        throw_exception("Inverter needs to have exactly one input.");
    }
    if (type != inverter_element && input_count < 2) [[unlikely]] {
        throw_exception("Input count needs to be at least 2 for standard elements.");
    }

    const auto element_id = layout_.add_logic_element(position, orientation);
    {
        const auto element = schematic_.add_element({
            .element_type = type,
            .input_count = input_count,
            .output_count = 1,
        });
        if (element.element_id() != element_id) [[unlikely]] {
            throw_exception("Added element ids don't match.");
        }
    }

    return connect_new_element(element_id);
}

auto EditableCircuit::add_wire(LineTree&& line_tree) -> element_id_t {
    auto delays = calculate_output_delays(line_tree);
    auto max_delay = std::ranges::max(delays);

    const auto element_id = layout_.add_wire(std::move(line_tree));
    {
        const auto element = schematic_.add_element({
            .element_type = ElementType::wire,
            .input_count = 1,
            .output_count = delays.size(),
            .output_delays = delays,
            .history_length = max_delay,
        });
        if (element.element_id() != element_id) [[unlikely]] {
            throw_exception("Added element ids don't match.");
        }
    }

    return connect_new_element(element_id);
}

auto EditableCircuit::swap_and_delete_element(element_id_t element_id) -> element_id_t {
    auto last_id1 = schematic_.swap_and_delete_element(element_id);
    auto last_id2 = layout_.swap_and_delete_element(element_id);

    if (last_id1 != last_id2) {
        throw_exception("Returned id's during deletion are not the same.");
    }
    return last_id1;
}

auto EditableCircuit::swap_and_delete_elements(std::span<const element_id_t> element_ids)
    -> void {
    // sort descending
    auto sorted_ids
        = folly::small_vector<element_id_t, 6> {element_ids.begin(), element_ids.end()};
    std::ranges::sort(sorted_ids, std::greater<> {});

    for (const auto element_id : sorted_ids) {
        swap_and_delete_element(element_id);
    }
}

auto EditableCircuit::connect_input(Schematic::Input input, point_t position)
    -> std::optional<element_id_t> {
    auto placeholder_id = std::optional<element_id_t> {};

    // pre-conditions
    if (input.has_connected_element()) [[unlikely]] {
        throw_exception("Input needs to be unconnected.");
    }
    if (input_connections_.contains(position)) [[unlikely]] {
        throw_exception("Circuit already contains input at this location.");
    }

    // connect to possible output
    if (const auto it = output_connections_.find(position);
        it != output_connections_.end()) {
        const auto found_output = schematic_.output(it->second);

        if (found_output.has_connected_element()) {
            if (found_output.connected_element().element_type()
                != ElementType::placeholder) [[unlikely]] {
                throw_exception("Output is already connected at this location.");
            }
            // mark placeholder for deletion
            placeholder_id = found_output.connected_element_id();
        }
        input.connect(found_output);
    }
    // register input
    input_connections_[position] = input;

    return placeholder_id;
}

auto EditableCircuit::connect_output(Schematic::Output output, point_t position)
    -> std::optional<element_id_t> {
    auto placeholder_id = std::optional<element_id_t> {};

    // pre-conditions
    if (output.has_connected_element()) [[unlikely]] {
        throw_exception("Output needs to be unconnected.");
    }
    if (output_connections_.contains(position)) [[unlikely]] {
        throw_exception("Circuit already contains output at this location.");
    }

    // connect to possible input
    if (const auto it = input_connections_.find(position);
        it != input_connections_.end()) {
        const auto found_input = schematic_.input(it->second);

        if (found_input.has_connected_element()) {
            if (found_input.connected_element().element_type()
                != ElementType::placeholder) [[unlikely]] {
                throw_exception("Input is already connected at this location.");
            }
            // mark placeholder for deletion
            placeholder_id = found_input.connected_element_id();
        }
        output.connect(found_input);
    }
    // register output
    output_connections_[position] = output;

    return placeholder_id;
}

auto EditableCircuit::connect_new_element(element_id_t element_id) -> element_id_t {
    folly::small_vector<element_id_t, 6> delete_queue {};

    // connect inputs
    for_each_input_locations(
        schematic_, layout_, element_id,
        [&, element_id, input_id = connection_id_t {0}](point_t position) mutable {
            const auto input = schematic_.element(element_id).input(input_id);

            const auto placeholder_id = connect_input(input, position);
            if (placeholder_id.has_value()) {
                delete_queue.push_back(placeholder_id.value());
            }

            ++input_id;
        });

    // connect outputs
    for_each_output_locations(
        schematic_, layout_, element_id,
        [&, element_id, output_id = connection_id_t {0}](point_t position) mutable {
            const auto output = schematic_.element(element_id).output(output_id);

            const auto placeholder_id = connect_output(output, position);
            if (placeholder_id.has_value()) {
                delete_queue.push_back(placeholder_id.value());
            }

            ++output_id;
        });

    add_missing_placeholders(element_id);

    // this invalidates our element_id
    swap_and_delete_elements(delete_queue);

    // TODO implement if needed
    return null_element;
}

auto EditableCircuit::add_missing_placeholders(element_id_t element_id) -> void {
    for (const auto output : schematic_.element(element_id).outputs()) {
        if (!output.has_connected_element()) {
            auto placeholder_id = add_placeholder_element();
            output.connect(schematic_.element(placeholder_id).input(connection_id_t {0}));
        }
    }
}

}  // namespace logicsim
