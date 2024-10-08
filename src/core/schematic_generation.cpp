#include "schematic_generation.h"

#include "algorithm/transform_to_container.h"
#include "geometry/orientation.h"
#include "index/connection_index.h"
#include "layout.h"
#include "line_tree.h"
#include "line_tree_generation.h"
#include "element/logicitem/schematic_info.h"
#include "schematic.h"
#include "vocabulary/output_delays.h"

#include <cassert>
#include <exception>
#include <stdexcept>

namespace logicsim {

namespace {

auto calculate_output_delays(const LineTree& line_tree,
                             delay_t wire_delay_per_distance) -> output_delays_t {
    const auto lengths = line_tree.calculate_output_lengths();

    return transform_to_container<output_delays_t>(
        lengths, [&](const length_t& length) -> delay_t {
            return wire_delay_per_distance * length.value;
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

auto logic_item_output_delays(const Layout& layout,
                              logicitem_id_t logicitem_id) -> output_delays_t {
    const auto logicitem_type = layout.logic_items().type(logicitem_id);
    const auto delay = element_output_delay(logicitem_type);

    switch (logicitem_type) {
        using enum LogicItemType;

        case clock_generator: {
            const auto& attrs = layout.logic_items().attrs_clock_generator(logicitem_id);
            if (attrs.is_symmetric) {
                return {delay, attrs.time_symmetric, attrs.time_symmetric};
            }
            return output_delays_t {delay, attrs.time_on, attrs.time_off};
        }

        default: {
            const auto output_count = layout.logic_items().output_count(logicitem_id);
            return {output_delays_t(output_count.count(), delay)};
        }
    }
}

auto add_logic_item(Schematic& schematic, const Layout& layout,
                    logicitem_id_t logicitem_id) -> void {
    schematic.add_element(schematic::NewElement {
        .element_type = to_element_type(layout.logic_items().type(logicitem_id)),
        .input_count = layout.logic_items().input_count(logicitem_id),
        .output_count = layout.logic_items().output_count(logicitem_id),

        .sub_circuit_id = layout.logic_items().sub_circuit_id(logicitem_id),
        .input_inverters = layout.logic_items().input_inverters(logicitem_id),
        .output_delays = logic_item_output_delays(layout, logicitem_id),
        .history_length = schematic::defaults::no_history,
    });
}

auto add_wire(Schematic& schematic, const Layout& layout, wire_id_t wire_id,
              const LineTree& line_tree, delay_t wire_delay_per_distance) -> void {
    if (line_tree.empty()) {
        const auto output_count = layout.wires().segment_tree(wire_id).output_count();

        schematic.add_element(schematic::NewElement {
            .element_type = ElementType::wire,
            .input_count = connection_count_t {0},
            .output_count = output_count,

            .sub_circuit_id = null_circuit,
            .input_inverters = {},
            .output_delays = output_delays_t(output_count.count(), delay_t::epsilon()),
            .history_length = schematic::defaults::no_history,
        });

    } else {
        auto ignore_delay = wire_delay_per_distance == delay_t {0ns};

        auto delays =
            ignore_delay
                ? output_delays_t(line_tree.output_count().count(), delay_t::epsilon())
                : calculate_output_delays(line_tree, wire_delay_per_distance);

        const auto tree_max_delay =
            ignore_delay ? delay_t {0ns} : std::ranges::max(delays);

        schematic.add_element(schematic::NewElement {
            .element_type = ElementType::wire,
            .input_count = connection_count_t {1},
            .output_count = line_tree.output_count(),

            .sub_circuit_id = null_circuit,
            .input_inverters = {false},
            .output_delays = std::move(delays),
            .history_length = tree_max_delay,
        });
    }
}

auto add_layout_elements(Schematic& schematic, const Layout& layout,
                         const std::vector<LineTree>& line_trees,
                         delay_t wire_delay_per_distance) -> void {
    // elements
    for (const auto logicitem_id : logicitem_ids(layout)) {
        if (is_inserted(layout, logicitem_id)) {
            add_logic_item(schematic, layout, logicitem_id);
        } else {
            add_unused_element(schematic);
        }
    }

    // non-inserted wires
    for (const auto _ [[maybe_unused]] : range(first_inserted_wire_id)) {
        add_unused_element(schematic);
    }

    // inserted wires
    for (const auto inserted_wire_id : inserted_wire_ids(layout)) {
        const auto& line_tree = line_trees.at(inserted_wire_id.value);

        add_wire(schematic, layout, inserted_wire_id, line_tree, wire_delay_per_distance);
    }
}

//
// Connections
//

struct GenerationCache {
    explicit GenerationCache(const Layout& layout) : inputs {layout}, outputs {layout} {}

    LogicItemInputIndex inputs;
    LogicItemOutputIndex outputs;
};

auto connect_line_tree(Schematic& schematic, const Layout& layout,
                       const GenerationCache& cache, element_id_t element_id,
                       const LineTree& line_tree) -> void {
    Expects(!line_tree.empty());

    // connect input
    {
        if (const auto entry = cache.outputs.find(line_tree.input_position())) {
            if (!orientations_compatible(entry->orientation,
                                         line_tree.input_orientation())) {
                throw std::runtime_error("input orientation not compatible");
            }
            const auto connected_element_id = to_element_id(layout, entry->logicitem_id);

            const auto input = input_t {element_id, connection_id_t {0}};
            const auto output = output_t {connected_element_id, entry->connection_id};
            schematic.connect(input, output);
        }
    }

    // connect outputs
    for (const output_t output : outputs(schematic, element_id)) {
        const auto position = line_tree.output_position(output.connection_id);
        if (const auto entry = cache.inputs.find(position)) {
            if (!orientations_compatible(entry->orientation, line_tree.output_orientation(
                                                                 output.connection_id))) {
                throw std::runtime_error("input orientation not compatible");
            }
            const auto connected_element_id = to_element_id(layout, entry->logicitem_id);

            const auto input = input_t {connected_element_id, entry->connection_id};
            schematic.connect(output, input);
        }
    }
}

// wires without inputs have no LineTree
auto connect_segment_tree(Schematic& schematic, const Layout& layout,
                          const GenerationCache& cache, element_id_t element_id) -> void {
    Expects(schematic.input_count(element_id) != connection_count_t {0});

    // connect outputs
    auto output_id = connection_id_t {0};
    const auto try_connect_output = [&](point_t position, orientation_t orientation) {
        if (const auto entry = cache.inputs.find(position)) {
            if (!orientations_compatible(entry->orientation, orientation)) {
                throw std::runtime_error("input orientation not compatible");
            }
            const auto connected_element_id = to_element_id(layout, entry->logicitem_id);

            const auto output = output_t {element_id, output_id};
            const auto input = input_t {connected_element_id, entry->connection_id};
            schematic.connect(output, input);

            ++output_id;
        }
    };

    for (const auto& info : layout.wires().segment_tree(to_wire_id(layout, element_id))) {
        if (info.p0_type == SegmentPointType::output) {
            try_connect_output(info.line.p0, to_orientation_p0(info.line));
        }
        if (info.p1_type == SegmentPointType::output) {
            try_connect_output(info.line.p1, to_orientation_p1(info.line));
        }
    }
}

auto create_connections(Schematic& schematic, const Layout& layout,
                        const std::vector<LineTree>& line_trees) -> void {
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
            const auto& line_tree = line_trees.at(to_wire_id(layout, element_id).value);

            if (line_tree.empty()) {
                connect_segment_tree(schematic, layout, cache, element_id);
            } else {
                connect_line_tree(schematic, layout, cache, element_id, line_tree);
            }
        }
    }
}

//
// Output Inverters
//

auto set_output_inverters(Schematic& schematic, const Layout& layout,
                          logicitem_id_t logicitem_id) -> void {
    const auto element_id = to_element_id(layout, logicitem_id);

    for (output_t output : outputs(schematic, element_id)) {
        if (layout.logic_items().output_inverted(logicitem_id, output.connection_id)) {
            // logic items are either connected to wires or output placeholders
            const auto input = schematic.input(output);
            assert(input);
            schematic.set_input_inverter(input, true);
        }
    }
}

auto set_output_inverters(Schematic& schematic, const Layout& layout) -> void {
    for (const auto logicitem_id : logicitem_ids(layout)) {
        if (is_inserted(layout, logicitem_id)) {
            set_output_inverters(schematic, layout, logicitem_id);
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

auto generate_schematic(const Layout& layout, const std::vector<LineTree>& line_trees,
                        delay_t wire_delay_per_distance) -> Schematic {
    assert(all_wires_equivalent(layout, line_trees));

    auto schematic = Schematic {};

    add_layout_elements(schematic, layout, line_trees, wire_delay_per_distance);
    create_connections(schematic, layout, line_trees);
    add_missing_placeholders(schematic);
    set_output_inverters(schematic, layout);

    return schematic;
}

auto to_element_id(const Layout& layout [[maybe_unused]],
                   logicitem_id_t logicitem_id) -> element_id_t {
    static_assert(std::is_same_v<element_id_t::value_type, logicitem_id_t::value_type>);

    return element_id_t {logicitem_id.value};
}

auto to_element_id(const Layout& layout, wire_id_t wire_id) -> element_id_t {
    static_assert(std::is_same_v<element_id_t::value_type, wire_id_t::value_type>);
    Expects(wire_id);

    const auto value = static_cast<int64_t>(layout.logic_items().size()) +
                       static_cast<int64_t>(wire_id.value);

    if (value > element_id_t::max().value) [[unlikely]] {
        throw std::runtime_error("overflow when generating element id");
    }
    return element_id_t {static_cast<element_id_t::value_type>(value)};
}

auto to_logicitem_id(const Layout& layout, element_id_t element_id) -> logicitem_id_t {
    static_assert(std::is_same_v<element_id_t::value_type, logicitem_id_t::value_type>);

    if (element_id.value >= std::ssize(layout.logic_items())) [[unlikely]] {
        throw std::runtime_error("not a logicitem id");
    }

    return logicitem_id_t {element_id.value};
}

auto to_wire_id(const Layout& layout, element_id_t element_id) -> wire_id_t {
    static_assert(std::is_same_v<element_id_t::value_type, wire_id_t::value_type>);

    const auto value = static_cast<int64_t>(element_id.value) -
                       static_cast<int64_t>(layout.logic_items().size());

    if (value < 0) [[unlikely]] {
        throw std::runtime_error("not a wire id");
    }
    return wire_id_t {static_cast<wire_id_t::value_type>(value)};
}

}  // namespace logicsim
