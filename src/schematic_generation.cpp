#include "schematic_generation.h"

#include "editable_circuit/caches/connection_cache.h"

namespace logicsim {

namespace {

auto add_placeholder_element(Schematic& schematic) -> Schematic::Element {
    constexpr static auto connector_delay
        = delay_t {Schematic::defaults::wire_delay_per_distance.value / 2};

    return schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::placeholder,
        .input_count = 1,
        .output_count = 0,
        .history_length = connector_delay,
    });
}

auto add_unused_element(Schematic& schematic) -> Schematic::Element {
    return schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::unused,
        .input_count = 0,
        .output_count = 0,

        .circuit_id = null_circuit,
        .input_inverters = {},
        .output_delays = {},
        .history_length = Schematic::defaults::no_history,
    });
}

auto add_logic_item(Schematic& schematic, layout::ConstElement element) -> void {
    schematic.add_element(Schematic::ElementData {
        .element_type = element.element_type(),
        .input_count = element.input_count(),
        .output_count = element.output_count(),

        .circuit_id = element.sub_circuit_id(),
        // .input_inverters = logic_small_vector_t(element.input_count(), false),
        .output_delays = std::vector<delay_t>(element.output_count(),
                                              Schematic::defaults::standard_delay),
        .history_length = Schematic::defaults::no_history,
    });
}

auto add_wire(Schematic& schematic, layout::ConstElement element) -> void {
    const auto& line_tree = element.line_tree();

    if (line_tree.empty()) {
        schematic.add_element(Schematic::ElementData {
            .element_type = element.element_type(),
            .input_count = 0,
            .output_count = 0,
        });

    } else {
        auto delays = calculate_output_delays(line_tree);
        const auto tree_max_delay = std::ranges::max(delays);

        schematic.add_element(Schematic::ElementData {
            .element_type = element.element_type(),
            .input_count = 1,
            .output_count = line_tree.output_count(),

            .circuit_id = null_circuit,
            // .input_inverters = logic_small_vector_t(1, false),
            .output_delays = std::move(delays),
            .history_length = tree_max_delay,
        });
    }
}

auto add_layout_elements(Schematic& schematic, const Layout& layout) -> void {
    for (const auto element : layout.elements()) {
        if (element.display_state() != display_state_t::normal) {
            add_unused_element(schematic);
        } else if (element.is_logic_item()) {
            add_logic_item(schematic, element);
        } else if (element.is_wire()) {
            add_wire(schematic, element);
        } else {
            add_unused_element(schematic);
        }
    }
}

auto create_connections(Schematic& schematic, const Layout& layout) -> void {
    // connection caches
    auto input_cache = ConnectionCache<true>();
    auto output_cache = ConnectionCache<false>();
    {
        add_logic_items_to_cache(input_cache, layout);
        add_logic_items_to_cache(output_cache, layout);
    }

    // connect wires to elements
    for (auto element : schematic.elements()) {
        const auto& line_tree = layout.line_tree(element.element_id());

        if (line_tree.empty()) {
            continue;
        }

        // connect input
        {
            auto output = output_cache.find(line_tree.input_position(), schematic);
            if (output.has_value()) {
                if (!orientations_compatible(output.value().second,
                                             line_tree.input_orientation())) {
                    throw_exception("input orientation not compatible");
                }
                output.value().first.connect(element.input(connection_id_t {0}));
            }
        }

        // connect outputs
        for (auto output : element.outputs()) {
            const auto output_index = output.output_index().value;
            auto input
                = input_cache.find(line_tree.output_position(output_index), schematic);

            if (input.has_value()) {
                if (!orientations_compatible(
                        input.value().second,
                        line_tree.output_orientation(output_index))) {
                    throw_exception("input orientation not compatible");
                }
                input.value().first.connect(output);
            }
        }
    }
}

auto add_missing_placeholders(Schematic& schematic, const Layout& layout) -> void {
    for (auto element : schematic.elements()) {
        if (element.is_logic_item()) {
            for (auto output : element.outputs()) {
                if (!output.has_connected_element()) {
                    auto placeholder = add_placeholder_element(schematic);
                    placeholder.input(connection_id_t {0}).connect(output);
                }
            }
        }
    }
}

}  // namespace

auto generate_schematic(const Layout& layout) -> Schematic {
    auto schematic = Schematic {layout.circuit_id()};

    add_layout_elements(schematic, layout);
    create_connections(schematic, layout);
    add_missing_placeholders(schematic, layout);

    return schematic;
}

}  // namespace logicsim
