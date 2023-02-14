#include "editable_circuit.h"

#include "exceptions.h"
#include "layout_calculations.h"

#include <fmt/core.h>

#include <algorithm>

namespace logicsim {

EditableCircuit::EditableCircuit(Schematic&& schematic, Layout&& layout)
    : schematic_ {std::move(schematic)}, layout_ {std::move(layout)} {}

auto EditableCircuit::layout() const noexcept -> const Layout& {
    return layout_;
}

auto EditableCircuit::schematic() const noexcept -> const Schematic& {
    return schematic_;
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
    for_each_input_locations(
        schematic_, layout_, element_id,
        [this, element_id, input_id = connection_id_t {0}](point_t position) mutable {
            const auto input = schematic_.element(element_id).input(input_id);
            fmt::print("{} - {}\n", input, position);
            ++input_id;
        });

    for_each_output_locations(
        schematic_, layout_, element_id,
        [this, element_id, output_id = connection_id_t {0}](point_t position) mutable {
            const auto output = schematic_.element(element_id).output(output_id);
            fmt::print("{} - {}\n", output, position);
            ++output_id;
        });
}

}  // namespace logicsim
