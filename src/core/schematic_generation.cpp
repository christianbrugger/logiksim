#include "core/schematic_generation.h"

#include "core/algorithm/transform_to_container.h"
#include "core/element/logicitem/schematic_info.h"
#include "core/geometry/orientation.h"
#include "core/geometry/segment_info.h"
#include "core/index/generation_index.h"
#include "core/layout.h"
#include "core/line_tree.h"
#include "core/line_tree_generation.h"
#include "core/schematic.h"
#include "core/vocabulary/output_delays.h"

#include <cassert>
#include <stdexcept>

namespace logicsim {

auto schematic_generation_result_t::format() const -> std::string {
    return fmt::format(
        "schematic_generation_result_t(\n"  //
        "  line_trees = {}\n"               //
        "  schematic = {}\n"                //
        "  wire_delay_per_distance = {}\n"  //
        ")",
        line_trees, schematic, wire_delay_per_distance);
}

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

auto logicitem_output_delays(const Layout& layout,
                             logicitem_id_t logicitem_id) -> output_delays_t {
    const auto logicitem_type = layout.logicitems().type(logicitem_id);
    const auto delay = element_output_delay(logicitem_type);

    switch (logicitem_type) {
        using enum LogicItemType;

        case clock_generator: {
            const auto& attrs = layout.logicitems().attrs_clock_generator(logicitem_id);
            if (attrs.is_symmetric) {
                return {delay, attrs.time_symmetric, attrs.time_symmetric};
            }
            return output_delays_t {delay, attrs.time_on, attrs.time_off};
        }

        default: {
            const auto output_count = layout.logicitems().output_count(logicitem_id);
            return {output_delays_t(output_count.count(), delay)};
        }
    }
}

auto add_logicitem(Schematic& schematic, const Layout& layout,
                   logicitem_id_t logicitem_id) -> void {
    schematic.add_element(schematic::NewElement {
        .element_type = to_element_type(layout.logicitems().type(logicitem_id)),
        .input_count = layout.logicitems().input_count(logicitem_id),
        .output_count = layout.logicitems().output_count(logicitem_id),

        .sub_circuit_id = layout.logicitems().sub_circuit_id(logicitem_id),
        .input_inverters = layout.logicitems().input_inverters(logicitem_id),
        .output_delays = logicitem_output_delays(layout, logicitem_id),
        .history_length = schematic::defaults::no_history,
    });
}

auto add_wire_with_input(Schematic& schematic, const LineTree& line_tree,
                         delay_t wire_delay_per_distance) -> void {
    Expects(!line_tree.empty());

    auto ignore_delay = wire_delay_per_distance == delay_t {0ns};
    auto delays =
        ignore_delay
            ? output_delays_t(line_tree.output_count().count(), delay_t::epsilon())
            : calculate_output_delays(line_tree, wire_delay_per_distance);
    const auto tree_max_delay = ignore_delay ? delay_t {0ns} : std::ranges::max(delays);

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

auto add_wire_without_input(Schematic& schematic,
                            const SegmentTree& segment_tree) -> void {
    Expects(!segment_tree.has_input());

    const auto output_count = segment_tree.output_count();

    schematic.add_element(schematic::NewElement {
        .element_type = ElementType::wire,
        .input_count = connection_count_t {0},
        .output_count = output_count,

        .sub_circuit_id = null_circuit,
        .input_inverters = {},
        .output_delays = output_delays_t(size_t {output_count}, delay_t {1ns}),
        .history_length = schematic::defaults::no_history,
    });
}

auto add_layout_elements(Schematic& schematic, const Layout& layout,
                         const std::vector<LineTree>& line_trees,
                         delay_t wire_delay_per_distance) -> void {
    // elements
    for (const auto logicitem_id : logicitem_ids(layout)) {
        if (is_inserted(layout, logicitem_id)) {
            add_logicitem(schematic, layout, logicitem_id);
        } else {
            add_unused_element(schematic);
        }
    }

    // non-inserted wires
    for (const auto wire_id : range(first_inserted_wire_id)) {
        if (std::size_t {wire_id} < layout.wires().size()) {
            add_unused_element(schematic);
        }
    }

    // inserted wires
    for (const auto inserted_wire_id : inserted_wire_ids(layout)) {
        const auto& line_tree = line_trees.at(inserted_wire_id.value);

        if (!line_tree.empty()) {
            add_wire_with_input(schematic, line_tree, wire_delay_per_distance);
        } else {
            add_wire_without_input(schematic,
                                   layout.wires().segment_tree(inserted_wire_id));
        }
    }
}

//
// Connections
//

auto connect_segment_tree_without_inputs(Schematic& schematic, const Layout& layout,
                                         const GenerationIndex& index,
                                         element_id_t element_id) -> void {
    const auto wire_id = to_wire_id(layout, element_id);
    const auto& segment_tree = layout.wires().segment_tree(wire_id);

    // trees with inputs should use generated line-trees, as its more efficient
    Expects(!segment_tree.has_input());

    auto wire_output_id = connection_id_t {0};

    // TODO !!! refactor loop

    // connect outputs
    for (const auto& segment : segment_tree.segments()) {
        for (auto&& [position, type, orientation] : to_point_type_orientation(segment)) {
            if (type != SegmentPointType::output) {
                continue;
            }
            const auto entry = index.inputs.find(position);
            if (!entry) {
                continue;
            }
            if (!orientations_compatible(orientation, entry->orientation)) {
                throw std::runtime_error("input orientation not compatible");
            }

            const auto connected_element_id = to_element_id(layout, entry->logicitem_id);
            const auto output = output_t {element_id, wire_output_id};
            const auto input = input_t {connected_element_id, entry->connection_id};

            schematic.connect(output, input);
            ++wire_output_id;
        }
    }

    Expects(size_t {wire_output_id} <= size_t {segment_tree.output_count()});
    Expects(size_t {wire_output_id} <= size_t {schematic.output_count(element_id)});
}

auto connect_line_tree(Schematic& schematic, const Layout& layout,
                       const GenerationIndex& index, element_id_t element_id,
                       const LineTree& line_tree) -> void {
    Expects(!line_tree.empty());

    // connect input
    {
        if (const auto entry = index.outputs.find(line_tree.input_position())) {
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
        if (const auto entry = index.inputs.find(position)) {
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

auto create_connections(Schematic& schematic, const Layout& layout,
                        const std::vector<LineTree>& line_trees,
                        const GenerationIndex& index) -> void {
    for (auto element_id : element_ids(schematic)) {
        const auto element_type = schematic.element_type(element_id);

        // internal connections
        for (const auto connection : element_internal_connections(element_type)) {
            schematic.connect(input_t {element_id, connection.input},
                              output_t {element_id, connection.output});
        }

        // connect wires to elements
        if (element_type == ElementType::wire) {
            const auto wire_id = to_wire_id(layout, element_id);

            if (const auto& line_tree = line_trees.at(wire_id.value);
                !line_tree.empty()) {
                connect_line_tree(schematic, layout, index, element_id, line_tree);
            } else {
                connect_segment_tree_without_inputs(schematic, layout, index, element_id);
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
        if (layout.logicitems().output_inverted(logicitem_id, output.connection_id)) {
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
        if (is_logicitem(schematic.element_type(element_id))) {
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
    -> schematic_generation_result_t {
    const auto index = GenerationIndex {layout};

    auto result = schematic_generation_result_t {
        .line_trees = generate_line_trees(layout),
        .schematic = Schematic {},
        .wire_delay_per_distance = wire_delay_per_distance,
    };

    add_layout_elements(result.schematic, layout, result.line_trees,
                        wire_delay_per_distance);
    create_connections(result.schematic, layout, result.line_trees, index);
    add_missing_placeholders(result.schematic);
    set_output_inverters(result.schematic, layout);

    Ensures(result.line_trees.size() == layout.wires().size());
    return result;
}

auto to_element_id(const Layout& layout [[maybe_unused]],
                   logicitem_id_t logicitem_id) -> element_id_t {
    static_assert(std::is_same_v<element_id_t::value_type, logicitem_id_t::value_type>);

    return element_id_t {logicitem_id.value};
}

auto to_element_id(const Layout& layout, wire_id_t wire_id) -> element_id_t {
    static_assert(std::is_same_v<element_id_t::value_type, wire_id_t::value_type>);
    Expects(wire_id);

    const auto value = static_cast<int64_t>(layout.logicitems().size()) +
                       static_cast<int64_t>(wire_id.value);

    if (value > element_id_t::max().value) [[unlikely]] {
        throw std::runtime_error("overflow when generating element id");
    }
    return element_id_t {static_cast<element_id_t::value_type>(value)};
}

auto to_logicitem_id(const Layout& layout, element_id_t element_id) -> logicitem_id_t {
    static_assert(std::is_same_v<element_id_t::value_type, logicitem_id_t::value_type>);

    if (element_id.value >= std::ssize(layout.logicitems())) [[unlikely]] {
        throw std::runtime_error("not a logicitem id");
    }

    return logicitem_id_t {element_id.value};
}

auto to_wire_id(const Layout& layout, element_id_t element_id) -> wire_id_t {
    static_assert(std::is_same_v<element_id_t::value_type, wire_id_t::value_type>);

    const auto value = static_cast<int64_t>(element_id.value) -
                       static_cast<int64_t>(layout.logicitems().size());

    if (value < 0) [[unlikely]] {
        throw std::runtime_error("not a wire id");
    }
    return wire_id_t {static_cast<wire_id_t::value_type>(value)};
}

}  // namespace logicsim
