#include "core/benchmark/render_line_scene.h"

#include "core/algorithm/accumulate.h"
#include "core/algorithm/range.h"
#include "core/algorithm/transform_to_vector.h"
#include "core/algorithm/uniform_int_distribution.h"
#include "core/editable_circuit.h"
#include "core/geometry/line.h"
#include "core/geometry/orientation.h"
#include "core/geometry/segment_info.h"
#include "core/line_tree.h"
#include "core/line_tree_generation.h"
#include "core/logging.h"
#include "core/random/bool.h"
#include "core/random/generator.h"
#include "core/random/ordered_line.h"
#include "core/render/circuit/render_background.h"
#include "core/render/circuit/render_circuit.h"
#include "core/render/render_context.h"
#include "core/schematic_generation.h"
#include "core/simulation_player.h"
#include "core/spatial_simulation.h"
#include "core/timer.h"
#include "core/tree_normalization.h"
#include "core/vocabulary/logicitem_definition.h"
#include "core/vocabulary/simulation_config.h"

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
        editable_circuit.add_wire_segment(ordered_line_t {line_t {view[0], view[1]}},
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
                             const RenderBenchmarkConfig& config) -> void {
    const auto grid_dist = get_udist(config.min_grid, config.max_grid, rng);
    const auto p0 = point_t {grid_dist(), grid_dist()};

    const auto as_horizontal = get_random_bool(rng);
    add_tree_segment(rng, editable_circuit, p0, as_horizontal, config);
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

auto get_random_wires(Rng& rng, const RenderBenchmarkConfig& config) -> Layout {
    auto editable_circuit = EditableCircuit {};

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

auto inserted_wire_lengths(const SpatialSimulation& spatial_simulation) -> int64_t {
    const auto tree_length = [&](const wire_id_t wire_id) {
        return calculate_tree_length(spatial_simulation.line_tree(wire_id));
    };

    return accumulate(inserted_wire_ids(spatial_simulation.layout()), int64_t {0},
                      tree_length);
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

// TODO refactor n_lines being unused
auto fill_line_scene(int n_lines [[maybe_unused]]) -> SimulatedLineScene {
    const auto config = RenderBenchmarkConfig();
    const auto simulation_config = SimulationConfig {.use_wire_delay = true};
    if (simulation_config.wire_delay_per_distance() != delay_t {1us}) {
        throw std::runtime_error("we depend on 1us for consistency");
    }

    auto rng = get_random_number_generator(0);

    // generate line_trees & layout
    auto spatial_simulation = SpatialSimulation {
        get_random_wires(rng, config), simulation_config.wire_delay_per_distance()};

    // simulated time
    const auto max_delay = maximum_output_delay(spatial_simulation.schematic());
    if (max_delay == delay_t {0ns}) {
        throw std::runtime_error("simulated time should not be zero");
    }

    // generate & submit events
    run_with_events(
        spatial_simulation.simulation(),
        generate_random_events(rng, spatial_simulation.schematic(), max_delay, config));

    // run simulation till the end
    const auto final_delay =
        (time_t::zero() + max_delay) - spatial_simulation.simulation().time();
    if (final_delay > delay_t::zero()) {
        spatial_simulation.simulation().run({.simulate_for = final_delay});
    }

    const auto wire_lengths = inserted_wire_lengths(spatial_simulation);

    return SimulatedLineScene {
        .spatial_simulation = std::move(spatial_simulation),
        .total_wire_length_sum = wire_lengths,
    };
}

auto benchmark_line_renderer(int n_lines, bool save_image) -> int64_t {
    auto scene = fill_line_scene(n_lines);

    const auto size = BLSizeI {1200, 1200};

    auto bl_image = BLImage {size.w, size.h, BL_FORMAT_PRGB32};
    const auto cache = ContextCache {};

    // TODO generate settings from bl_image, ...
    const auto settings = [&] {
        auto res = ContextRenderSettings {.thread_count = ThreadCount::synchronous};
        res.view_config.set_device_scale(12.);
        res.view_config.set_size(size);
        return res;
    }();

    render_to_image(bl_image, settings, cache, [&](Context& ctx) {
        render_background(ctx);

        const auto timer = Timer {"Render", Timer::Unit::ms, 3};
        render_simulation(ctx, scene.spatial_simulation);
    });

    if (save_image) {
        bl_image.writeToFile("benchmark_line_renderer.png");
    }

    return scene.total_wire_length_sum;
}

}  // namespace logicsim
