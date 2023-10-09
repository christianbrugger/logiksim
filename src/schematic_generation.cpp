#include "schematic_generation.h"

#include "editable_circuit/cache/connection_cache.h"
#include "editable_circuit/cache/helper.h"
#include "geometry/orientation.h"
#include "layout_calculation.h"
#include "simulation_type.h"

namespace logicsim {

namespace {

//
// Layout Elements
//

auto add_placeholder_element(Schematic& schematic) -> Schematic::Element {
    return schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::placeholder,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {0},
        .history_length = Schematic::no_history,
    });
}

auto add_unused_element(Schematic& schematic) -> void {
    schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::unused,
        .input_count = connection_count_t {0},
        .output_count = connection_count_t {0},

        .sub_circuit_id = null_circuit,
        .input_inverters = {},
        .output_delays = {},
        .history_length = Schematic::no_history,
    });
}

auto add_logic_item(Schematic& schematic, layout::ConstElement element) -> void {
    const auto output_delays = [&]() -> std::vector<delay_t> {
        switch (element.element_type()) {
            using enum ElementType;

            case button: {
                return {defaults::button_delay};
            }

            case clock_generator: {
                const auto& attrs = element.attrs_clock_generator();
                if (attrs.is_symmetric) {
                    return {delay_t::epsilon(), attrs.time_symmetric,
                            attrs.time_symmetric};
                }
                return {delay_t::epsilon(), attrs.time_on, attrs.time_off};
            }

            default: {
                return std::vector<delay_t>(element.output_count().count(),
                                            defaults::logic_item_delay);
            }
        }
        throw_exception("invalid");
    }();

    schematic.add_element(Schematic::ElementData {
        .element_type = element.element_type(),
        .input_count = element.input_count(),
        .output_count = element.output_count(),

        .sub_circuit_id = element.sub_circuit_id(),
        .input_inverters = element.input_inverters(),
        .output_delays = output_delays,
        .history_length = Schematic::no_history,
    });
}

auto add_wire(Schematic& schematic, layout::ConstElement element) -> void {
    const auto& line_tree = element.line_tree();

    if (line_tree.empty()) {
        // TODO: temporarily disable wires with to many outputs
        if (element.output_count() > connection_count_t::max()) {
            add_unused_element(schematic);
        } else {
            const auto output_count = element.segment_tree().output_count();

            schematic.add_element(Schematic::ElementData {
                .element_type = element.element_type(),
                .input_count = connection_count_t {0},
                .output_count = output_count,
                .output_delays =
                    std::vector<delay_t>(output_count.count(), delay_t::epsilon()),
            });
        }

    } else {
        auto ignore_delay = schematic.wire_delay_per_distance() == delay_t {0ns};

        auto delays =
            ignore_delay
                ? std::vector<delay_t>(line_tree.output_count().count(),
                                       delay_t::epsilon())
                : calculate_output_delays(line_tree, schematic.wire_delay_per_distance());
        const auto tree_max_delay =
            ignore_delay ? delay_t {0ns} : std::ranges::max(delays);

        // TODO: temporarily disable wires with to many outputs
        if (line_tree.output_count() > connection_count_t::max()) {
            add_unused_element(schematic);
        } else {
            schematic.add_element(Schematic::ElementData {
                .element_type = element.element_type(),
                .input_count = connection_count_t {1},
                .output_count = line_tree.output_count(),

                .sub_circuit_id = null_circuit,
                .output_delays = std::move(delays),
                .history_length = tree_max_delay,
            });
        }
    }
}

auto add_layout_elements(Schematic& schematic, const Layout& layout) -> void {
    for (const auto element : layout.elements()) {
        bool inserted = element.is_inserted();

        if (inserted && element.is_logic_item()) {
            add_logic_item(schematic, element);
        }

        else if (inserted && element.is_wire()) {
            add_wire(schematic, element);
        }

        else {
            add_unused_element(schematic);
        }
    }
}

//
// Connections
//

struct GenerationCache {
    GenerationCache(const Layout& layout) {
        add_logic_items_to_cache(inputs, layout);
        add_logic_items_to_cache(outputs, layout);
    }

    ConnectionCache<true> inputs {};
    ConnectionCache<false> outputs {};
};

auto connect_line_tree(const GenerationCache& cache, const LineTree& line_tree,
                       Schematic::Element element) -> void {
    auto& schematic = element.schematic();

    // connect input
    {
        if (const auto entry = cache.outputs.find(line_tree.input_position())) {
            if (!orientations_compatible(entry->orientation,
                                         line_tree.input_orientation())) {
                throw_exception("input orientation not compatible");
            }
            const auto output = schematic.output(entry->connection());
            output.connect(element.input(connection_id_t {0}));
        }
    }

    // connect outputs
    for (auto output : element.outputs()) {
        const auto output_index = output.output_index();
        const auto position = line_tree.output_position(output_index);

        if (const auto entry = cache.inputs.find(position)) {
            if (!orientations_compatible(entry->orientation,
                                         line_tree.output_orientation(output_index))) {
                throw_exception("input orientation not compatible");
            }
            const auto input = schematic.input(entry->connection());
            input.connect(output);
        }
    }
}

// wires without inputs have no LineTree
auto connect_segment_tree(const GenerationCache& cache, const SegmentTree& segment_tree,
                          Schematic::Element element) -> void {
    if (element.input_count() != connection_count_t {0}) [[unlikely]] {
        throw_exception("can only connect segment trees without inputs");
    }
    auto& schematic = element.schematic();

    // connect outputs
    auto output_index = connection_id_t {0};
    const auto try_connect_output = [&](point_t position, orientation_t orientation) {
        if (const auto entry = cache.inputs.find(position)) {
            if (!orientations_compatible(entry->orientation, orientation)) {
                throw_exception("input orientation not compatible");
            }

            const auto output = element.output(output_index);
            const auto input = schematic.input(entry->connection());
            input.connect(output);

            ++output_index;
        }
    };

    for (const auto& info : segment_tree.segment_infos()) {
        if (info.p0_type == SegmentPointType::output) {
            try_connect_output(info.line.p0, to_orientation_p0(info.line));
        }
        if (info.p1_type == SegmentPointType::output) {
            try_connect_output(info.line.p1, to_orientation_p1(info.line));
        }
    }
}

auto create_connections(Schematic& schematic, const Layout& layout) -> void {
    const auto cache = GenerationCache {layout};

    for (auto element : schematic.elements()) {
        // connect clock generator internals
        if (element.element_type() == ElementType::clock_generator) {
            element.input(connection_id_t {1})
                .connect(element.output(connection_id_t {1}));
            element.input(connection_id_t {2})
                .connect(element.output(connection_id_t {2}));
            continue;
        }

        // connect wires to elements
        if (element.element_type() == ElementType::wire) {
            const auto& line_tree = layout.line_tree(element);
            if (!line_tree.empty()) {
                connect_line_tree(cache, line_tree, element);
            } else {
                connect_segment_tree(cache, layout.segment_tree(element), element);
            }
        }
    }
}

//
// Missing Placeholders
//

auto add_missing_placeholders(Schematic& schematic) -> void {
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

//
// Output Inverters
//

auto set_output_inverters(Schematic& schematic, layout::ConstElement element) -> void {
    for (auto output : schematic.element(element).outputs()) {
        if (element.output_inverted(output.output_index())) {
            output.connected_input().set_inverted(true);
        }
    }
}

auto set_output_inverters(Schematic& schematic, const Layout& layout) -> void {
    for (const auto element : layout.elements()) {
        if (element.is_inserted() && element.is_logic_item()) {
            set_output_inverters(schematic, element);
        }
    }
}

}  // namespace

//
// Main
//

auto generate_schematic(const Layout& layout, delay_t wire_delay_per_unit) -> Schematic {
    auto schematic = Schematic {layout.circuit_id(), wire_delay_per_unit};

    add_layout_elements(schematic, layout);
    create_connections(schematic, layout);
    add_missing_placeholders(schematic);
    set_output_inverters(schematic, layout);

    return schematic;
}

}  // namespace logicsim
