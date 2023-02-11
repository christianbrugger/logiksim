#include "editable_circuit.h"

#include <algorithm>

namespace logicsim {

EditableCircuit::EditableCircuit(Schematic&& schematic, CircuitLayout&& layout)
    : schematic_ {std::move(schematic)}, layout_ {std::move(layout)} {}

auto EditableCircuit::layout() const noexcept -> const CircuitLayout& {
    return layout_;
}

auto EditableCircuit::schematic() const noexcept -> const Schematic& {
    return schematic_;
}

auto EditableCircuit::add_standard_element(ElementType type, std::size_t input_count,
                                           point_t position,
                                           DisplayOrientation orientation) -> void {
    using enum ElementType;
    if (!(type == and_element || type == or_element || type == xor_element)) {
        throw_exception("The type needs to be standard element.");
    }

    schematic_.add_element({.element_type = type, .input_count = input_count});
    layout_.add_logic_element(position, orientation);
}

auto EditableCircuit::add_wire(LineTree&& line_tree) -> void {
    auto delays = calculate_output_delays(line_tree);
    auto max_delay = std::ranges::max(delays);

    schematic_.add_element({.element_type = ElementType::wire,
                            .output_count = delays.size(),
                            .output_delays = delays,
                            .history_length = max_delay});
    layout_.add_wire(std::move(line_tree));
}

}  // namespace logicsim
