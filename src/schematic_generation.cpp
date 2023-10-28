#include "schematic_generation.h"

#include "algorithm/transform_to_container.h"
#include "editable_circuit/cache/connection_cache.h"
#include "editable_circuit/cache/helper.h"
#include "exception.h"
#include "geometry/orientation.h"
#include "logging.h"
#include "logic_item/schematic_info.h"
#include "schematic.h"
#include "vocabulary/output_delays.h"

#include <cassert>
#include <exception>

namespace logicsim {

namespace {

auto calculate_output_delays(const LineTree& line_tree, delay_t wire_delay_per_distance)
    -> output_delays_t {
    auto lengths = line_tree.calculate_output_lengths();

    return transform_to_container<output_delays_t>(
        lengths, [&](length_t length) -> delay_t {
            return wire_delay_per_distance * length.value;  // TODO don't use length.value
        });
}

//
// Layout Elements
//

auto add_placeholder_element(Schematic& schematic) -> element_id_t {
    return schematic.add_element(schematic::NewElement {
        .element_type = ElementType::placeholder,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {0},

        .sub_circuit_id = null_circuit,
        .input_inverters = {false},
        .output_delays = {},
        .history_length = schematic::defaults::no_history,
    });
}

auto add_unused_element(Schematic& schematic) -> void {
    schematic.add_element(schematic::NewElement {
        .element_type = ElementType::unused,
        .input_count = connection_count_t {0},
        .output_count = connection_count_t {0},

        .sub_circuit_id = null_circuit,
        .input_inverters = {},
        .output_delays = {},
        .history_length = schematic::defaults::no_history,
    });
}

auto logic_item_output_delays(layout::ConstElement element) -> output_delays_t {
    const auto delay = element_output_delay(element.element_type());

    switch (element.element_type()) {
        using enum ElementType;

        case clock_generator: {
            const auto& attrs = element.attrs_clock_generator();
            if (attrs.is_symmetric) {
                return {delay, attrs.time_symmetric, attrs.time_symmetric};
            }
            return output_delays_t {delay, attrs.time_on, attrs.time_off};
        }

        default: {
            return output_delays_t(element.output_count().count(), delay);
        }
    }
    std::terminate();
}

auto add_logic_item(Schematic& schematic, layout::ConstElement element) -> void {
    schematic.add_element(schematic::NewElement {
        .element_type = element.element_type(),
        .input_count = element.input_count(),
        .output_count = element.output_count(),

        .sub_circuit_id = element.sub_circuit_id(),
        .input_inverters = element.input_inverters(),
        .output_delays = logic_item_output_delays(element),
        .history_length = schematic::defaults::no_history,
    });
}

auto add_wire(Schematic& schematic, layout::ConstElement element,
              delay_t wire_delay_per_distance) -> void {
    const auto& line_tree = element.line_tree();

    if (line_tree.empty()) {
        // TODO: temporarily disable wires with to many outputs
        if (element.output_count() > connection_count_t::max()) {
            add_unused_element(schematic);
        } else {
            const auto output_count = element.segment_tree().output_count();

            schematic.add_element(schematic::NewElement {
                .element_type = element.element_type(),
                .input_count = connection_count_t {0},
                .output_count = output_count,

                .sub_circuit_id = null_circuit,
                .input_inverters = {},
                .output_delays =
                    output_delays_t(output_count.count(), delay_t::epsilon()),
                .history_length = schematic::defaults::no_history,
            });
        }

    } else {
        auto ignore_delay = wire_delay_per_distance == delay_t {0ns};

        auto delays =
            ignore_delay
                ? output_delays_t(line_tree.output_count().count(), delay_t::epsilon())
                : calculate_output_delays(line_tree, wire_delay_per_distance);
        const auto tree_max_delay =
            ignore_delay ? delay_t {0ns} : std::ranges::max(delays);

        // TODO: temporarily disable wires with to many outputs
        if (line_tree.output_count() > connection_count_t::max()) {
            add_unused_element(schematic);
        } else {
            schematic.add_element(schematic::NewElement {
                .element_type = element.element_type(),
                .input_count = connection_count_t {1},
                .output_count = line_tree.output_count(),

                .sub_circuit_id = null_circuit,
                .input_inverters = {false},
                .output_delays = std::move(delays),
                .history_length = tree_max_delay,
            });
        }
    }
}

auto add_layout_elements(Schematic& schematic, const Layout& layout,
                         delay_t wire_delay_per_distance) -> void {
    for (const auto element : layout.elements()) {
        bool inserted = element.is_inserted();

        if (inserted && element.is_logic_item()) {
            add_logic_item(schematic, element);
        }

        else if (inserted && element.is_wire()) {
            add_wire(schematic, element, wire_delay_per_distance);
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

auto connect_line_tree(Schematic& schematic, element_id_t element_id,
                       const LineTree& line_tree, const GenerationCache& cache) -> void {
    // connect input
    {
        if (const auto entry = cache.outputs.find(line_tree.input_position())) {
            if (!orientations_compatible(entry->orientation,
                                         line_tree.input_orientation())) {
                throw_exception("input orientation not compatible");
            }
            const auto input = input_t {element_id, connection_id_t {0}};
            const auto output = output_t {entry->element_id, entry->connection_id};
            schematic.connect(input, output);
        }
    }

    // connect outputs
    for (const output_t output : outputs(schematic, element_id)) {
        const auto position = line_tree.output_position(output.connection_id);
        if (const auto entry = cache.inputs.find(position)) {
            if (!orientations_compatible(entry->orientation, line_tree.output_orientation(
                                                                 output.connection_id))) {
                throw_exception("input orientation not compatible");
            }

            const auto input = input_t {entry->element_id, entry->connection_id};
            schematic.connect(output, input);
        }
    }
}

// wires without inputs have no LineTree
auto connect_segment_tree(Schematic& schematic, element_id_t element_id,
                          const SegmentTree& segment_tree, const GenerationCache& cache)
    -> void {
    if (schematic.input_count(element_id) != connection_count_t {0}) [[unlikely]] {
        throw_exception("wires with inputs should have a line tree");
    }

    // connect outputs
    auto output_id = connection_id_t {0};
    const auto try_connect_output = [&](point_t position, orientation_t orientation) {
        if (const auto entry = cache.inputs.find(position)) {
            if (!orientations_compatible(entry->orientation, orientation)) {
                throw_exception("input orientation not compatible");
            }

            const auto output = output_t {element_id, output_id};
            const auto input = input_t {entry->element_id, entry->connection_id};
            schematic.connect(output, input);

            ++output_id;
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

    for (auto element_id : element_ids(schematic)) {
        const auto element_type = schematic.element_type(element_id);

        // internal connections
        for (const auto element : element_internal_connections(element_type)) {
            schematic.connect(input_t {element_id, element.input},
                              output_t {element_id, element.output});
        }

        // connect wires to elements
        if (element_type == ElementType::wire) {
            const auto& line_tree = layout.line_tree(element_id);
            if (!line_tree.empty()) {
                connect_line_tree(schematic, element_id, line_tree, cache);
            } else {
                connect_segment_tree(schematic, element_id,
                                     layout.segment_tree(element_id), cache);
            }
        }
    }
}

//
// Output Inverters
//

auto set_output_inverters(Schematic& schematic, layout::ConstElement element) -> void {
    for (output_t output : outputs(schematic, element)) {
        if (element.output_inverted(output.connection_id)) {
            // logic items are either connected to wires or output placeholders
            const auto input = schematic.input(output);
            assert(input);
            schematic.set_input_inverter(input, true);
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
// Missing Placeholders
//

auto add_missing_placeholders(Schematic& schematic) -> void {
    for (auto element_id : element_ids(schematic)) {
        if (is_logic_item(schematic.element_type(element_id))) {
            for (auto output : outputs(schematic, element_id)) {
                if (!schematic.input(output)) {
                    const auto placeholder_id = add_placeholder_element(schematic);
                    const auto input = input_t {placeholder_id, connection_id_t {0}};
                    schematic.connect(output, input);
                }
            }
        }
    }
}

//
// Generate Schematic
//

auto generate_schematic(const Layout& layout, delay_t wire_delay_per_distance)
    -> Schematic {
    auto schematic = Schematic {};

    add_layout_elements(schematic, layout, wire_delay_per_distance);
    create_connections(schematic, layout);
    add_missing_placeholders(schematic);
    set_output_inverters(schematic, layout);

    return schematic;
}

}  // namespace logicsim
