#include "circuit_index.h"

#include "exceptions.h"
#include "range.h"

#include <algorithm>

namespace logicsim {

auto validate_connections(Schematic::ConstElement element,
                          display_state_t display_state) {
    if (is_inserted(display_state)) {
        if (element.is_wire()) {
            validate_has_no_placeholders(element);
        } else {
            validate_all_outputs_connected(element);
        }
    } else {
        validate_all_inputs_disconnected(element);
        validate_all_outputs_disconnected(element);
    }
}

auto validate_placeholder_display_state(Schematic::ConstElement element,
                                        display_state_t display_state) {
    if (element.is_placeholder() && !is_inserted(display_state)) {
        throw_exception("placeholder has wrong display state");
    }
}

auto validate_trees_match_wires(Schematic::ConstElement element, const Layout& layout) {
    const auto element_id = element.element_id();

    const auto& segment_tree = layout.segment_tree(element_id);
    const auto& line_tree = layout.line_tree(element_id);

    if (!element.is_wire()) {
        if (!line_tree.empty() || !segment_tree.empty()) [[unlikely]] {
            throw_exception("non-wire element cannot have line or segment trees.");
        }
    }

    else {  // is_wire
        // segment and line trees are compared in layout::validate(), so we don't
        // need to do this here

        if (layout.position(element_id) != point_t {}) [[unlikely]] {
            throw_exception("wires should not have a position");
        }
        if (layout.orientation(element_id) != orientation_t::undirected) [[unlikely]] {
            throw_exception("wires should not have a orientation");
        }

        if (segment_tree.empty()) [[unlikely]] {
            throw_exception("found wire without elements");
        }
        // if (segment_tree.output_count() != element.output_count()) {
        //     throw_exception("output counts don't match");
        // }
        if (segment_tree.input_count() != element.input_count()) {
            throw_exception("input counts don't match");
        }

        // TODO compare
        // - output_count
        // - output_delays
        // - history_length
    }
}

auto validate(const Layout& layout, const Schematic& schematic) -> void {
    // layout & schematic
    layout.validate();
    schematic.validate(Schematic::ValidationSettings {
        .require_all_outputs_connected = false,
        .require_all_placeholders_connected = true,
    });

    // global attributes
    if (layout.circuit_id() != schematic.circuit_id()) [[unlikely]] {
        throw_exception("layout and circuit have different circuit ids");
    }
    if (layout.element_count() != schematic.element_count()) [[unlikely]] {
        throw_exception("layout and elements need to have same element count");
    }

    // elements consistent
    for (const auto element : schematic.elements()) {
        const auto element_id = element.element_id();
        const auto display_state = layout.display_state(element_id);

        // connections
        validate_connections(element, display_state);
        validate_placeholder_display_state(element, display_state);
        // wires & trees
        validate_trees_match_wires(element, layout);
    }
}

//
// Circuit Index
//

auto CircuitIndex::circuit_count() const -> std::size_t {
    check_equal_size();
    return schematics_.size();
}

auto CircuitIndex::borrow_schematic(circuit_id_t circuit_id) -> Schematic {
    auto& source = schematics_.at(circuit_id.value);

    if (source.circuit_id() != circuit_id) [[unlikely]] {
        throw_exception("Cannot borrow missing schematics.");
    }
    auto result = Schematic {null_circuit};
    source.swap(result);
    return result;
}

auto CircuitIndex::borrow_schematics() -> std::vector<Schematic> {
    check_are_schematics_complete();

    auto result = std::vector<Schematic>(schematics_.size(), Schematic {null_circuit});
    schematics_.swap(result);
    return result;
}

auto CircuitIndex::borrow_layout(circuit_id_t circuit_id) -> Layout {
    auto& source = layouts_.at(circuit_id.value);

    if (source.circuit_id() != circuit_id) [[unlikely]] {
        throw_exception("Cannot borrow missing layout.");
    }
    auto result = Layout {null_circuit};
    source.swap(result);
    return result;
}

auto CircuitIndex::return_schematic(Schematic&& schematic) -> void {
    auto& source = schematics_.at(schematic.circuit_id().value);

    if (source.circuit_id() != null_circuit) [[unlikely]] {
        throw_exception("Cannot return ocupied schematic.");
    }

    source.swap(schematic);
}

auto CircuitIndex::return_schematics(std::vector<Schematic>&& schematics) -> void {
    if (schematics.size() != schematics_.size()) {
        throw_exception("Cannot return different number of schematics, than borrowed.");
    }
    if (!std::ranges::all_of(schematics_, [](const Schematic& schematic) {
            return schematic.circuit_id() == null_circuit;
        })) {
        throw_exception("Cannot return occupied schematic.");
    }
    if (!std::ranges::equal(
            schematics, range(schematics.size()), {},
            [](const Schematic& schematic) { return schematic.circuit_id().value; })) {
        throw_exception("Circuit-ids need to have correct order.");
    }

    schematics_.swap(schematics);
}

auto CircuitIndex::return_layout(Layout&& layout) -> void {
    auto& source = layouts_.at(layout.circuit_id().value);

    if (source.circuit_id() != null_circuit) [[unlikely]] {
        throw_exception("Cannot return ocupied layout.");
    }

    source.swap(layout);
}

auto CircuitIndex::description(circuit_id_t circuit_id) -> const CircuitDescription& {
    return descriptions_.at(circuit_id.value);
}

auto CircuitIndex::descriptions() -> const std::vector<CircuitDescription>& {
    return descriptions_;
}

auto CircuitIndex::check_is_complete() const -> void {
    check_equal_size();

    check_are_schematics_complete();
    check_are_layouts_complete();
    check_are_descriptions_complete();
}

auto CircuitIndex::check_are_schematics_complete() const -> void {
    if (!std::ranges::equal(
            schematics_, range(schematics_.size()), {},
            [](const Schematic& item) { return item.circuit_id().value; })) {
        throw_exception("Some schematics are missing.");
    }
}

auto CircuitIndex::check_are_layouts_complete() const -> void {
    if (!std::ranges::equal(layouts_, range(layouts_.size()), {},
                            [](const Layout& item) { return item.circuit_id().value; })) {
        throw_exception("Some layouts are missing.");
    }
}

auto CircuitIndex::check_are_descriptions_complete() const -> void {
    if (!std::ranges::equal(
            descriptions_, range(descriptions_.size()), {},
            [](const CircuitDescription& item) { return item.circuit_id.value; })) {
        throw_exception("Some descriptions are missing.");
    }
}

auto CircuitIndex::check_equal_size() const -> void {
    if (!(schematics_.size() == layouts_.size() == descriptions_.size())) {
        throw_exception("Schematics, layouts and descriptions have different sizes.");
    }
}
}  // namespace logicsim