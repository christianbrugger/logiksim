
#include "editable_circuit/caches/connection_cache.h"
#include "editable_circuit/editable_circuit.h"
#include "renderer.h"
#include "simulation.h"
#include "timer.h"

#include <fmt/core.h>

#include <exception>
#include <iostream>

namespace logicsim {

auto print_circuit_stats(const Circuit& circuit) {
    const auto& schematic = circuit.schematic();
    const auto& layout = circuit.layout();

    auto element_count = std::size_t {0};
    auto segment_count = std::size_t {0};

    for (auto element : schematic.elements()) {
        if (element.is_wire()) {
            const auto& tree = layout.segment_tree(element.element_id());
            segment_count += tree.segment_count();
        }

        else if (element.is_logic_item()) {
            ++element_count;
        }
    }

    if (schematic.element_count() < 10) {
        print(circuit);
    }
    fmt::print("Circuit with {} elements and {} wire segments.\n", element_count,
               segment_count);
}

auto generate_circuit() -> Circuit {
    const auto timer = Timer {"Generate", Timer::Unit::ms, 2};

#ifdef NDEBUG
    constexpr auto debug_build = false;
#else
    constexpr auto debug_build = true;
#endif
    constexpr auto debug_max = 50;
    constexpr auto release_max = 1600;  //  1600;

    constexpr auto max_value = debug_build ? debug_max : release_max;

    auto editable_circuit = EditableCircuit {Circuit {Schematic {}, Layout {}}};

    for (auto x : range(5, max_value, 5)) {
        for (auto y : range(5, max_value, 5)) {
            editable_circuit.add_standard_logic_item(ElementType::or_element, 3,
                                                     point_t {grid_t {x}, grid_t {y}},
                                                     InsertionMode::insert_or_discard);

            editable_circuit.add_line_segments(point_t {grid_t {x + 2}, grid_t {y + 1}},
                                               point_t {grid_t {x + 4}, grid_t {y - 1}},
                                               LineSegmentType::horizontal_first,
                                               InsertionMode::insert_or_discard);

            editable_circuit.add_line_segments(point_t {grid_t {x + 3}, grid_t {y + 1}},
                                               point_t {grid_t {x + 5}, grid_t {y + 2}},
                                               LineSegmentType::vertical_first,
                                               InsertionMode::insert_or_discard);
        }
    }

    return editable_circuit.extract_circuit();
}

struct GeneratedSchematic {
    Schematic schematic {};
    std::vector<LineTree> line_trees;
};

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

auto convert_circuit(const Circuit& circuit) -> GeneratedSchematic {
    auto input_cache = ConnectionCache<true>();
    auto output_cache = ConnectionCache<false>();
    {
        const auto t = Timer {"Fill Caches", Timer::Unit::ms, 2};
        add_circuit_to_cache(input_cache, circuit);
        add_circuit_to_cache(output_cache, circuit);
    }

    // start

    const auto t0 = Timer {"Convert", Timer::Unit::ms, 2};

    const auto& schematic = circuit.schematic();
    const auto& layout = circuit.layout();

    auto generated = GeneratedSchematic {};

    // Generate Line Trees
    {
        const auto t = Timer {"  Generate Line Trees", Timer::Unit::ms, 2};

        generated.line_trees.resize(schematic.element_count());

        for (const auto element_id : layout.element_ids()) {
            const auto element = schematic.element(element_id);

            if (layout.display_state(element_id) != display_state_t::normal) {
                continue;
            }

            if (element.is_wire()) {
                auto& line_tree = generated.line_trees.at(element_id.value);
                line_tree = LineTree::from_segment_tree(layout.segment_tree(element_id))
                                .value();
                if (line_tree.empty()) {
                    throw_exception("line tree cannot be empty");
                }
            }
        }
    }

    // Add Schematic Elements
    {
        const auto t = Timer {"  Add Schematic Elements", Timer::Unit::ms, 2};

        for (const auto element_id : layout.element_ids()) {
            const auto element = schematic.element(element_id);

            if (layout.display_state(element_id) != display_state_t::normal) {
                add_unused_element(generated.schematic);
            }

            else if (element.is_logic_item()) {
                generated.schematic.add_element(Schematic::ElementData {
                    .element_type = element.element_type(),
                    .input_count = element.input_count(),
                    .output_count = element.output_count(),

                    .circuit_id = element.sub_circuit_id(),
                    .input_inverters = logic_small_vector_t(element.input_count(), false),
                    .output_delays = std::vector<delay_t>(
                        element.output_count(), Schematic::defaults::standard_delay),
                    .history_length = Schematic::defaults::no_history,
                });
            }

            else if (element.is_wire()) {
                const auto& line_tree = generated.line_trees.at(element_id.value);

                auto delays = calculate_output_delays(line_tree);
                const auto tree_max_delay = std::ranges::max(delays);

                generated.schematic.add_element(Schematic::ElementData {
                    .element_type = element.element_type(),
                    .input_count = 1,
                    .output_count = line_tree.output_count(),

                    .circuit_id = null_circuit,
                    .input_inverters = logic_small_vector_t(1, false),
                    .output_delays = std::move(delays),
                    .history_length = tree_max_delay,
                });
            }

            else {
                add_unused_element(generated.schematic);
            }
        }
    }

    // Connect Wires
    {
        const auto t = Timer {"  Connect Wires", Timer::Unit::ms, 2};
        for (auto element : generated.schematic.elements()) {
            if (!element.is_wire()) {
                continue;
            }

            const auto& line_tree = generated.line_trees.at(element.element_id().value);

            // connect input
            {
                auto output
                    = output_cache.find(line_tree.input_position(), generated.schematic);
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
                auto input = input_cache.find(line_tree.output_position(output_index),
                                              generated.schematic);

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

    // Add Placeholders
    {
        const auto t = Timer {"  Add Placeholders", Timer::Unit::ms, 2};
        for (auto element : generated.schematic.elements()) {
            if (element.is_logic_item()) {
                for (auto output : element.outputs()) {
                    if (!output.has_connected_element()) {
                        auto placeholder = add_placeholder_element(generated.schematic);
                        placeholder.input(connection_id_t {0}).connect(output);
                    }
                }
            }
        }
    }

    return generated;
}

auto benchmark_conversion() -> void {
    const auto circuit = generate_circuit();

    const auto converted = convert_circuit(circuit);

    print();
    print_circuit_stats(circuit);
    print();
    print("Circuit   Element Count:", circuit.schematic().element_count());
    print("Generated Element Count:", converted.schematic.element_count());

    // print();
    // print(converted.schematic);
}

}  // namespace logicsim

auto main() -> int {
    using namespace logicsim;

    /// TODO consider: ios_base::sync_with_stdio(false);
    /// SL.io.10 in https://isocpp.github.io/CppCoreGuidelines/

    benchmark_conversion();

    /*
    try {
        auto timer = Timer {"Benchmark", Timer::Unit::ms, 3};
        // auto count = benchmark_simulation(6, 10, true);
        // auto count = logicsim::benchmark_simulation(BENCHMARK_DEFAULT_ELEMENTS,
        //                                            BENCHMARK_DEFAULT_EVENTS, true);

        auto count = logicsim::benchmark_line_renderer(100, true);
        fmt::print("count = {}\n", count);

    } catch (const std::exception& exc) {
        std::cerr << exc.what() << '\n';
        return -1;
    }
    */
    return 0;
}
