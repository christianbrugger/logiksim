#include "benchmark/render_line_scene.h"

#include "algorithm/accumulate.h"
#include "algorithm/range.h"
#include "algorithm/transform_to_vector.h"
#include "algorithm/uniform_int_distribution.h"
#include "editable_circuit/editable_circuit.h"
#include "geometry/line.h"
#include "geometry/orientation.h"
#include "geometry/segment_info.h"
#include "line_tree_generation.h"
#include "logging.h"
#include "random/generator.h"
#include "random/ordered_line.h"
#include "render_circuit.h"
#include "schematic_generation.h"
#include "simulation_player.h"
#include "simulation_view.h"
#include "timer.h"
#include "tree_normalization.h"
#include "vocabulary/element_definition.h"

#include <range/v3/view/sliding.hpp>

#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace logicsim {

namespace {

struct RenderBenchmarkConfig {
    grid_t min_grid {1};
    grid_t max_grid {99};

    grid_t max_segment_length {5};

    std::size_t min_line_segments {1};
    std::size_t max_line_segments {5};

    int n_outputs_min {1};
    int n_outputs_max {5};

    int min_event_spacing_ns {500};
    int max_event_spacing_ns {3000};
};

auto get_udist(grid_t a, grid_t b, Rng& rng) {
    return [a, b, &rng]() -> grid_t {
        return grid_t {uint_distribution<grid_t::value_type>(a.value, b.value)(rng)};
    };
}

auto random_segment_value(grid_t last, const RenderBenchmarkConfig& config, Rng& rng) {
    auto grid_dist =
        get_udist(std::max(config.min_grid, last - config.max_segment_length),
                  std::min(config.max_grid, last + config.max_segment_length), rng);

    grid_t res;
    while ((res = grid_dist()) == last) {
    }
    return res;
}

auto new_line_point(point_t origin, bool horizontal, const RenderBenchmarkConfig& config,
                    Rng& rng) -> point_t {
    if (horizontal) {
        return point_t {random_segment_value(origin.x, config, rng), origin.y};
    }
    return point_t {origin.x, random_segment_value(origin.y, config, rng)};
}

auto new_line_point(point_t origin, point_t previous, const RenderBenchmarkConfig& config,
                    Rng& rng) -> point_t {
    return new_line_point(origin, is_vertical(line_t {previous, origin}), config, rng);
}

auto insert_tree_from_points(EditableCircuit& editable_circuit,
                             const std::vector<point_t>& points) -> void {
    for (auto&& view : ranges::views::sliding(points, 2)) {
        editable_circuit.add_line_segment(line_t {view[0], view[1]},
                                          InsertionMode::insert_or_discard);
    }
}

auto add_tree_segment(Rng& rng, EditableCircuit& editable_circuit, point_t start_point,
                      bool horizontal, const RenderBenchmarkConfig& config) -> void {
    auto segment_count_dist = uint_distribution<std::size_t>(config.min_line_segments,
                                                             config.max_line_segments);
    const auto n_segments = segment_count_dist(rng);

    auto points = std::vector<point_t> {
        start_point,
        new_line_point(start_point, horizontal, config, rng),
    };
    std::generate_n(std::back_inserter(points), n_segments - 1, [&]() {
        return new_line_point(points.back(), *(points.end() - 2), config, rng);
    });

    insert_tree_from_points(editable_circuit, points);
}

auto add_random_wire_segment(Rng& rng, EditableCircuit& editable_circuit,
                             RenderBenchmarkConfig config) -> void {
    const auto grid_dist = get_udist(config.min_grid, config.max_grid, rng);
    const auto p0 = point_t {grid_dist(), grid_dist()};

    const auto is_horizontal = uint_distribution<int>(0, 1)(rng);
    add_tree_segment(rng, editable_circuit, p0, is_horizontal, config);
}

auto set_inputs(Layout& layout) {
    for (const wire_id_t wire_id : wire_ids(layout)) {
        auto& m_tree = layout.wires().modifiable_segment_tree(wire_id);

        if (m_tree.empty()) {
            continue;
        }

        // find segment with output
        const auto it =
            std::ranges::find_if(m_tree.indices(), [&](const segment_index_t index) {
                const auto& info = m_tree.info(index);
                return info.p0_type == SegmentPointType::output ||
                       info.p1_type == SegmentPointType::output;
            });
        Expects(it != m_tree.indices().end());

        // change one output to an input
        auto new_info = m_tree.info(*it);
        if (new_info.p0_type == SegmentPointType::output) {
            new_info.p0_type = SegmentPointType::input;
        } else if (new_info.p1_type == SegmentPointType::output) {
            new_info.p1_type = SegmentPointType::input;
        }
        m_tree.update_segment(*it, new_info);
    }
}

auto get_random_wires(Rng& rng, RenderBenchmarkConfig config) -> Layout {
    auto editable_circuit = EditableCircuit {Layout {}};

    for (auto _ [[maybe_unused]] : range(400)) {
        add_random_wire_segment(rng, editable_circuit, config);
    }

    auto layout = editable_circuit.extract_layout();
    set_inputs(layout);
    return layout;
}

auto calculate_tree_length(const LineTree& line_tree) -> int {
    return accumulate(line_tree, int {0},
                      [](line_t line) -> int { return distance(line); });
}

auto inserted_wire_lengths(const Layout& layout) -> int64_t {
    return accumulate(inserted_wire_ids(layout), int64_t {0},
                      [&](const wire_id_t wire_id) {
                          return calculate_tree_length(layout.wires().line_tree(wire_id));
                      });
}

auto maximum_output_delay(const auto& schematic) -> delay_t {
    delay_t max_delay {0us};

    for (const auto element_id : element_ids(schematic)) {
        for (auto output : outputs(schematic, element_id)) {
            max_delay = std::max(max_delay, schematic.output_delay(output));
        }
    }

    return max_delay;
}

struct new_event_t {
    input_t input;
    delay_t offset;
    bool value;
};

auto generate_random_events(Rng& rng, const Schematic& schematic, delay_t max_delay,
                            const RenderBenchmarkConfig& config)
    -> std::vector<simulation::simulation_event_t> {
    auto events = std::vector<simulation::simulation_event_t> {};

    for (const auto element_id : element_ids(schematic)) {
        if (schematic.element_type(element_id) == ElementType::wire) {
            auto spacing_dist_ns = uint_distribution<int>(config.min_event_spacing_ns,
                                                          config.max_event_spacing_ns);
            bool next_value = true;
            auto next_delay = delay_t {spacing_dist_ns(rng) * 1ns};

            while (next_delay < max_delay) {
                events.push_back({
                    .time = time_t::zero() + next_delay,
                    .element_id = element_id,
                    .input_id = connection_id_t {0},
                    .value = next_value,
                });

                next_value = next_value ^ true;
                next_delay = next_delay + delay_t {spacing_dist_ns(rng) * 1ns};
            }
        }
    }

    return events;
}

}  // namespace

auto fill_line_scene(int n_lines) -> SimulatedLineScene {
    const auto config = RenderBenchmarkConfig();
    const auto simulation_settings = SimulationSettings {.use_wire_delay = true};
    if (simulation_settings.wire_delay_per_distance() != delay_t {1us}) {
        throw std::runtime_error("we depend on 1us for consistency");
    }

    auto rng = get_random_number_generator(0);

    // generate line_trees & layout
    const auto layout = get_random_wires(rng, config);

    auto simulation = Simulation {
        generate_schematic(layout, simulation_settings.wire_delay_per_distance())};

    // simulated time
    const auto max_delay = maximum_output_delay(simulation.schematic());
    if (max_delay == delay_t {0ns}) {
        throw std::runtime_error("simulated time should not be zero");
    }

    // generate & submit events
    run_with_events(simulation, generate_random_events(rng, simulation.schematic(),
                                                       max_delay, config));

    // run simulation till the end
    const auto final_delay = (time_t::zero() + max_delay) - simulation.time();
    if (final_delay > delay_t::zero()) {
        simulation.run({.simulate_for = final_delay});
    }

    const auto wire_lengths = inserted_wire_lengths(layout);

    return SimulatedLineScene {
        .layout = std::move(layout),
        .simulation = std::move(simulation),
        .total_wire_length_sum = wire_lengths,
        .wire_delay_per_distance = simulation_settings.wire_delay_per_distance(),
    };
}

auto benchmark_line_renderer(int n_lines, bool save_image) -> int64_t {
    auto scene = fill_line_scene(n_lines);

    // render image
    auto circuit_ctx =
        CircuitContext {Context {.bl_image = BLImage {1200, 1200, BL_FORMAT_PRGB32},
                                 .settings = {.thread_count = 0}}};
    circuit_ctx.ctx.settings.view_config.set_device_scale(12.);
    auto& ctx = circuit_ctx.ctx;

    ctx.begin();
    render_background(ctx);
    {
        auto timer = Timer {"Render", Timer::Unit::ms, 3};
        render_simulation(
            circuit_ctx, scene.layout,
            SimulationView {scene.simulation, scene.wire_delay_per_distance});
    }
    ctx.end();

    if (save_image) {
        ctx.bl_image.writeToFile("benchmark_line_renderer.png");
    }

    return scene.total_wire_length_sum;
}

}  // namespace logicsim
