#include "circuit.h"

#include "format.h"

#include <fmt/core.h>

namespace logicsim {

Circuit::Circuit(Schematic&& schematic, Layout&& layout)
    : schematic_ {std::move(schematic)}, layout_ {std::move(layout)} {};

auto Circuit::format() const -> std::string {
    return fmt::format("{}\n{}\n", schematic(), layout());
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
    auto temp = std::move(schematic_.value());
    schematic_ = std::nullopt;
    return std::move(temp);
};

auto Circuit::extract_layout() -> Layout {
    auto temp = std::move(layout_.value());
    layout_ = std::nullopt;
    return std::move(temp);
};

// validation

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

auto Circuit::validate() const -> void {
    logicsim::validate(layout(), schematic());
}

}  // namespace logicsim