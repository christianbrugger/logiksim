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

auto EditableCircuit::add_standard_element(ElementType type, std::size_t input_count,
                                           point_t position,
                                           DisplayOrientation orientation) -> void {
    using enum ElementType;
    if (!(type == and_element || type == or_element || type == xor_element))
        [[unlikely]] {
        throw_exception("The type needs to be standard element.");
    }
    if (input_count < 2) [[unlikely]] {
        throw_exception("Input count needs to be at least 2 for standard elements.");
    }

    const auto element = schematic_.add_element(
        {.element_type = type, .input_count = input_count, .output_count = 1});
    const auto element_id = layout_.add_logic_element(position, orientation);

    if (element.element_id() != element_id) [[unlikely]] {
        throw_exception("Added element ids don't match.");
    }

    connect_new_element(element);
}

auto EditableCircuit::add_wire(LineTree&& line_tree) -> void {
    auto delays = calculate_output_delays(line_tree);
    auto max_delay = std::ranges::max(delays);

    const auto element = schematic_.add_element({.element_type = ElementType::wire,
                                                 .output_count = delays.size(),
                                                 .output_delays = delays,
                                                 .history_length = max_delay});
    const auto element_id = layout_.add_wire(std::move(line_tree));

    if (element.element_id() != element_id) [[unlikely]] {
        throw_exception("Added element ids don't match.");
    }

    connect_new_element(element);
}

auto EditableCircuit::connect_new_element(element_id_t element_id) -> void {
    // inputs
    for_each_input_locations(
        schematic_, layout_, element_id,
        [this, element_id, input_id = connection_id_t {0}](point_t point) mutable {
            const auto input = schematic_.element(element_id).input(input_id);
            // pre-conditions
            if (input.has_connected_element()) [[unlikely]] {
                throw_exception("Input needs to be unconnected.");
            }
            if (input_connections_.contains(point)) [[unlikely]] {
                throw_exception("Circuit already contains input at this location.");
            }

            // connect to possible output
            if (const auto it = output_connections_.find(point);
                it != output_connections_.end()) {
                const auto output = schematic_.output(it->second);
                if (output.has_connected_element()) [[unlikely]] {
                    throw_exception("Output is already connected at this location.");
                }
                input.connect(output);
                fmt::print("connected input {} to output {}\n", input, output);
                // TODO handle placeholders
            }
            // register input
            input_connections_[point] = connection_t {element_id, input_id};
            ++input_id;
        });

    // outputs
    for_each_output_locations(
        schematic_, layout_, element_id,
        [this, element_id, output_id = connection_id_t {0}](point_t point) mutable {
            const auto output = schematic_.element(element_id).output(output_id);
            // pre-conditions
            if (output.has_connected_element()) [[unlikely]] {
                throw_exception("Output needs to be unconnected.");
            }
            if (output_connections_.contains(point)) [[unlikely]] {
                throw_exception("Circuit already contains output at this location.");
            }

            // connect to possible input
            if (const auto it = input_connections_.find(point);
                it != input_connections_.end()) {
                const auto input = schematic_.input(it->second);
                if (input.has_connected_element()) [[unlikely]] {
                    throw_exception("Input is already connected at this location.");
                }
                output.connect(input);
                fmt::print("connected output {} to input {}\n", output, input);
                // TODO handle placeholders
            }
            // register output
            output_connections_[point] = connection_t {element_id, output_id};
            ++output_id;
        });
}

}  // namespace logicsim
