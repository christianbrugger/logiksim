#include "circuit.h"

#include "format.h"

#include <fmt/core.h>

namespace logicsim {

auto is_inserted(const Circuit& circuit, element_id_t element_id) -> bool {
    return is_inserted(circuit.layout(), element_id);
}

auto is_logic_item(const Circuit& circuit, element_id_t element_id) -> bool {
    return circuit.schematic().element(element_id).is_logic_item();
}

auto is_wire(const Circuit& circuit, element_id_t element_id) -> bool {
    return circuit.schematic().element(element_id).is_wire();
}

auto get_segment_info(const Circuit& circuit, segment_t segment) -> segment_info_t {
    return circuit.layout()
        .segment_tree(segment.element_id)
        .segment_info(segment.segment_index);
}

//
// Circuit
//

Circuit::Circuit(Schematic&& schematic, Layout&& layout)
    : schematic_ {std::move(schematic)}, layout_ {std::move(layout)} {};

auto Circuit::format() const -> std::string {
    return fmt::format("{}\n{}", schematic(), layout());
}

auto Circuit::schematic() const -> const Schematic& {
    return schematic_.value();
};

auto Circuit::layout() const -> const Layout& {
    return layout_.value();
};

auto Circuit::schematic() -> Schematic& {
    return schematic_.value();
};

auto Circuit::layout() -> Layout& {
    return layout_.value();
};

auto Circuit::extract_schematic() -> Schematic {
    auto temp = Schematic {std::move(schematic_.value())};
    schematic_ = std::nullopt;
    return temp;
};

auto Circuit::extract_layout() -> Layout {
    auto temp = Layout {std::move(layout_.value())};
    layout_ = std::nullopt;
    return temp;
}

auto Circuit::swap_elements(element_id_t element_id_0, element_id_t element_id_1)
    -> void {
    auto& _schematic = schematic();
    auto& _layout = layout();

    _schematic.swap_elements(element_id_0, element_id_1);
    _layout.swap_elements(element_id_0, element_id_1);
};

// validation

auto validate_connections(Schematic::ConstElement element,
                          display_state_t display_state) {
    if (is_inserted(display_state)) {
        validate_has_no_placeholders(element);
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

auto validate_wires_data(Schematic::ConstElement element, const Layout& layout) {
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

        if (segment_tree.empty() && is_inserted(layout, element.element_id()))
            [[unlikely]] {
            throw_exception("found inserted wire without elements");
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

auto count_wires(const Schematic& schematic, const Layout& layout,
                 display_state_t display_state) {
    return std::ranges::count_if(layout.element_ids(), [&](element_id_t element_id) {
        return schematic.element(element_id).is_wire()
               && layout.display_state(element_id) == display_state;
    });
}

// we store temporary and colliding wire segments all in one tree.
// we never have more than one in a circuit
auto validate_single_aggregate_trees(const Schematic& schematic, const Layout& layout)
    -> void {
    if (count_wires(schematic, layout, display_state_t::temporary) > 1) [[unlikely]] {
        throw_exception("found more than one aggregate temporary segment tree");
    }

    if (count_wires(schematic, layout, display_state_t::colliding) > 1) [[unlikely]] {
        throw_exception("found more than one aggregate temporary segment tree");
    }
}

auto validate_duplicate_data(const Schematic& schematic, const Layout& layout) -> void {
    for (auto elem1 : schematic.elements()) {
        auto elem2 = layout.element(elem1.element_id());

        if (elem1.element_type() != elem2.element_type()) {
            throw_exception("element_type missmatch");
        }
        if (elem1.input_count() != elem2.input_count()) {
            throw_exception("input_count missmatch");
        }
        if (elem1.output_count() != elem2.output_count()) {
            throw_exception("output_count missmatch");
        }
        if (elem1.sub_circuit_id() != elem2.sub_circuit_id()) {
            throw_exception("sub_circuit_id missmatch");
        }
    }
}

auto validate(const Schematic& schematic, const Layout& layout) -> void {
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

    // duplication
    validate_duplicate_data(schematic, layout);

    // elements consistent
    for (const auto element : schematic.elements()) {
        const auto element_id = element.element_id();
        const auto display_state = layout.display_state(element_id);

        // connections
        validate_connections(element, display_state);
        validate_placeholder_display_state(element, display_state);
        // wires & trees
        validate_wires_data(element, layout);
    }

    // wire aggregates
    validate_single_aggregate_trees(schematic, layout);
}

auto Circuit::validate() const -> void {
    logicsim::validate(schematic(), layout());
}

}  // namespace logicsim