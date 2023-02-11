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
        throw_exception("The ype needs to be standard element.");
    }

    schematic_.add_element({.element_type = type, .input_count = input_count});
    layout_.add_logic_element(position, orientation);
}

auto calculate_output_delays(const LineTree& line_tree) -> std::vector<delay_t> {
    auto lengths = line_tree.calculate_output_lengths();
    return transform_to_vector(lengths, [](LineTree::length_t length) -> delay_t {
        // TODO handle overflow
        return delay_t {Schematic::defaults::wire_delay_per_distance.value * length};
    });
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
