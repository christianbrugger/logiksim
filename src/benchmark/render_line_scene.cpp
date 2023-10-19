#include "benchmark/render_line_scene.h"

#include "algorithm/range.h"
#include "algorithm/transform_to_vector.h"
#include "algorithm/uniform_int_distribution.h"
#include "geometry/orientation.h"
#include "random/generator.h"
#include "render_circuit.h"
#include "simulation_view.h"
#include "timer.h"
#include "vocabulary/element_definition.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace logicsim {

struct RenderBenchmarkConfig {
    grid_t min_grid {1};
    grid_t max_grid {99};

    grid_t max_segment_length {5};

    std::size_t min_line_segments {1};
    std::size_t max_line_segments {5};

    int n_outputs_min {1};
    int n_outputs_max {5};

    int min_event_spacing_us {5};
    int max_event_spacing_us {30};
};

namespace {

template <std::uniform_random_bit_generator G>
auto get_udist(grid_t a, grid_t b, G& rng) {
    return [a, b, &rng]() -> grid_t {
        return grid_t {uint_distribution<grid_t::value_type>(a.value, b.value)(rng)};
    };
}

template <std::uniform_random_bit_generator G>
auto random_segment_value(grid_t last, const RenderBenchmarkConfig& config, G& rng) {
    // auto grid_dist = boost::random::uniform_int_distribution<grid_t> {
    //     std::max(config.min_grid, last - config.max_segment_length),
    //     std::min(config.max_grid, last + config.max_segment_length)};

    auto grid_dist =
        get_udist(std::max(config.min_grid, last - config.max_segment_length),
                  std::min(config.max_grid, last + config.max_segment_length), rng);

    grid_t res;
    while ((res = grid_dist()) == last) {
    }
    return res;
}

template <std::uniform_random_bit_generator G>
auto new_line_point(point_t origin, bool horizontal, const RenderBenchmarkConfig& config,
                    G& rng) -> point_t {
    if (horizontal) {
        return point_t {random_segment_value(origin.x, config, rng), origin.y};
    }
    return point_t {origin.x, random_segment_value(origin.y, config, rng)};
}

template <std::uniform_random_bit_generator G>
auto new_line_point(point_t origin, point_t previous, const RenderBenchmarkConfig& config,
                    G& rng) -> point_t {
    return new_line_point(origin, is_vertical(line_t {previous, origin}), config, rng);
}

// pick random point on line
template <std::uniform_random_bit_generator G>
auto pick_line_point(ordered_line_t line, G& rng) -> point_t {
    // return point2d_t {UDist<grid_t> {line.p0.x, line.p1.x}(rng),
    //                   UDist<grid_t> {line.p0.y, line.p1.y}(rng)};
    return point_t {get_udist(line.p0.x, line.p1.x, rng)(),
                    get_udist(line.p0.y, line.p1.y, rng)()};
}

template <std::uniform_random_bit_generator G>
auto create_line_tree_segment(point_t start_point, bool horizontal,
                              const RenderBenchmarkConfig& config, G& rng) -> LineTree {
    auto segment_count_dist = uint_distribution<std::size_t>(config.min_line_segments,
                                                             config.max_line_segments);
    auto n_segments = segment_count_dist(rng);

    auto line_tree = std::optional<LineTree> {};
    do {
        auto points = std::vector<point_t> {
            start_point,
            new_line_point(start_point, horizontal, config, rng),
        };
        std::generate_n(std::back_inserter(points), n_segments - 1, [&]() {
            return new_line_point(points.back(), *(points.end() - 2), config, rng);
        });

        line_tree = LineTree::from_points(points);
    } while (!line_tree.has_value());

    assert(line_tree->segment_count() == n_segments);
    return std::move(line_tree.value());
}

template <std::uniform_random_bit_generator G>
auto create_first_line_tree_segment(const RenderBenchmarkConfig& config, G& rng)
    -> LineTree {
    // const auto grid_dist = UDist<grid_t> {config.min_grid, config.max_grid};
    const auto grid_dist = get_udist(config.min_grid, config.max_grid, rng);
    const auto p0 = point_t {grid_dist(), grid_dist()};

    const auto is_horizontal = uint_distribution<int>(0, 1)(rng);
    return create_line_tree_segment(p0, is_horizontal, config, rng);
}

template <std::uniform_random_bit_generator G>
auto create_random_line_tree(connection_count_t n_outputs,
                             const RenderBenchmarkConfig& config, G& rng) -> LineTree {
    auto line_tree = create_first_line_tree_segment(config, rng);

    while (line_tree.output_count() < n_outputs) {
        auto new_tree = std::optional<LineTree> {};
        // TODO flatten loop
        do {
            const auto segment_index = uint_distribution<int>(
                0, gsl::narrow<int>(line_tree.segment_count()) - 1)(rng);
            const auto segment = line_tree.segment(segment_index);
            const auto origin = pick_line_point(ordered_line_t {segment}, rng);

            const auto sub_tree =
                create_line_tree_segment(origin, is_vertical(segment), config, rng);
            new_tree = merge({line_tree, sub_tree});
        } while (!new_tree.has_value());

        line_tree = std::move(new_tree.value());
    }

    return line_tree;
}

auto calculate_tree_length(const LineTree& line_tree) -> int {
    return std::transform_reduce(line_tree.segments().begin(), line_tree.segments().end(),
                                 0, std::plus {},
                                 [](line_t line) { return distance(line); });
}

}  // namespace

auto fill_line_scene(BenchmarkScene& scene, int n_lines) -> int64_t {
    auto rng = get_random_number_generator(0);
    const auto config = RenderBenchmarkConfig();
    auto tree_length_sum = int64_t {0};

    // create initial schematics
    auto& schematic = scene.schematic;
    for (auto _ [[maybe_unused]] : range(n_lines)) {
        const auto output_dist =
            uint_distribution<int>(config.n_outputs_min, config.n_outputs_max);
        // TODO can we simplify this?
        const auto output_count = connection_count_t {
            gsl::narrow<connection_count_t::value_type>(output_dist(rng))};

        schematic.add_element(SchematicOld::ElementData {
            .element_type = ElementType::wire,
            .input_count = connection_count_t {1},
            .output_count = output_count,
            .output_delays =
                std::vector<delay_t>(output_count.count(), defaults::logic_item_delay),
        });
    }
    add_output_placeholders(schematic);

    // create layout
    auto& layout = scene.layout = Layout {};
    for (auto element : schematic.elements()) {
        const auto definition = ElementDefinition {
            .element_type = element.element_type(),
            .input_count = element.input_count(),
            .output_count = element.output_count(),
        };
        layout.add_element(definition, point_t {}, display_state_t::normal);
    }

    // add line trees
    auto& simulation = scene.simulation = Simulation {schematic};
    for (auto element : schematic.elements()) {
        if (element.element_type() == ElementType::wire) {
            auto line_tree_gen =
                create_random_line_tree(element.output_count(), config, rng);

            // segment tree
            auto& m_tree = layout.modifyable_segment_tree(element);
            auto is_first = bool {true};

            for (const auto& entry : line_tree_gen.sized_segments()) {
                auto p0_type = is_first ? SegmentPointType::input
                                        : (entry.has_cross_point_p0
                                               ? SegmentPointType::cross_point
                                               : SegmentPointType::shadow_point);
                auto p1_type = SegmentPointType::shadow_point;

                if (entry.line.p0 > entry.line.p1) {
                    std::swap(p0_type, p1_type);
                }

                m_tree.add_segment(segment_info_t {.line = ordered_line_t {entry.line},
                                                   .p0_type = p0_type,
                                                   .p1_type = p1_type});

                is_first = false;
            }
            const auto& line_tree = layout.line_tree(element);

            // delays
            auto lengths = line_tree.calculate_output_lengths();
            assert(connection_count_t {lengths.size()} == element.output_count());
            auto delays =
                transform_to_vector(lengths, [&](LineTree::length_t length) -> delay_t {
                    return delay_t {schematic.wire_delay_per_distance() * length};
                });
            element.set_output_delays(delays);

            // history
            auto tree_max_delay = std::ranges::max(delays);
            element.set_history_length(tree_max_delay);

            // sum
            tree_length_sum += calculate_tree_length(line_tree);
        }
    }

    // init simulation
    simulation.initialize();

    // calculate simulation time
    delay_t max_delay {0us};
    for (auto element : schematic.elements()) {
        for (auto output : element.outputs()) {
            max_delay = std::max(max_delay, output.delay());
        }
    }
    if (max_delay == delay_t {0ns}) {
        throw std::runtime_error("delay should not be zero");
    }
    auto max_time = max_delay;

    // add events
    for (auto element : schematic.elements()) {
        if (element.element_type() == ElementType::wire) {
            auto spacing_dist_us = uint_distribution<int>(config.min_event_spacing_us,
                                                          config.max_event_spacing_us);
            bool next_value = true;
            auto next_time = delay_t {spacing_dist_us(rng) * 1us};

            while (next_time < max_time) {
                simulation.submit_event(element.input(connection_id_t {0}), next_time,
                                        next_value);

                next_value = next_value ^ true;
                next_time = next_time + delay_t {spacing_dist_us(rng) * 1us};
            }
        }
    }

    // run simulation
    simulation.run(max_time);

    return tree_length_sum;
}

auto benchmark_line_renderer(int n_lines, bool save_image) -> int64_t {
    BenchmarkScene scene;

    auto tree_length_sum = fill_line_scene(scene, n_lines);

    // render image
    auto circuit_ctx =
        CircuitContext {Context {.bl_image = BLImage {1200, 1200, BL_FORMAT_PRGB32}}};
    auto& ctx = circuit_ctx.ctx;

    ctx.begin();
    render_background(ctx);
    {
        auto timer = Timer {"Render", Timer::Unit::ms, 3};
        render_simulation(circuit_ctx, scene.layout, SimulationView {scene.simulation});
    }
    ctx.end();

    if (save_image) {
        ctx.bl_image.writeToFile("benchmark_line_renderer.png");
    }

    return tree_length_sum;
}

}  // namespace logicsim