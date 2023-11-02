#include "benchmark/render_line_scene.h"

#include "algorithm/accumulate.h"
#include "algorithm/range.h"
#include "algorithm/transform_to_vector.h"
#include "algorithm/uniform_int_distribution.h"
#include "geometry/orientation.h"
#include "random/generator.h"
#include "render_circuit.h"
#include "schematic_generation.h"
#include "simulation_player.h"
#include "simulation_view.h"
#include "timer.h"
#include "vocabulary/element_definition.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace logicsim {

/*

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
return accumulate(line_tree, int {0},
                  [](line_t line) -> int { return distance(line); });
}

auto total_wire_lengths(const Layout& layout) -> int64_t {
return accumulate(layout.elements(), int64_t {0},
                  [](const layout::ConstElement element) {
                      return calculate_tree_length(element.line_tree());
                  });
}

auto to_segment_tree(const LineTree& line_tree) -> SegmentTree {
auto segment_tree = SegmentTree {};
auto is_first = bool {true};

for (const auto& entry : line_tree.sized_segments()) {
    auto p0_type = is_first
                       ? SegmentPointType::input
                       : (entry.has_cross_point_p0 ? SegmentPointType::cross_point
                                                   : SegmentPointType::shadow_point);
    auto p1_type = SegmentPointType::shadow_point;

    if (entry.line.p0 > entry.line.p1) {
        std::swap(p0_type, p1_type);
    }

    segment_tree.add_segment(segment_info_t {
        .line = ordered_line_t {entry.line},
        .p0_type = p0_type,
        .p1_type = p1_type,
    });

    is_first = false;
}

return segment_tree;
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
    throw std::runtime_error("unexpected wire delay");
}

auto rng = get_random_number_generator(0);

// generate output counts
auto output_counts = std::vector<connection_count_t> {};
for (auto _ [[maybe_unused]] : range(n_lines)) {
    const auto output_count_dist =
        uint_distribution<int>(config.n_outputs_min, config.n_outputs_max);
    output_counts.push_back(connection_count_t {output_count_dist(rng)});
}

// generate line_trees & layout
auto layout = Layout {};
for (const auto& output_count : output_counts) {
    const auto line_tree = create_random_line_tree(output_count, config, rng);

    const auto element = layout.add_element(
        ElementDefinition {
            .element_type = ElementType::wire,
            .input_count = connection_count_t {0},
            .output_count = output_count,
            .orientation = orientation_t::undirected,
        },
        point_t {}, display_state_t::normal);

    auto& m_tree = layout.modifyable_segment_tree(element);
    m_tree = to_segment_tree(line_tree);
}

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

const auto wire_lengths = total_wire_lengths(layout);

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

*/

}  // namespace logicsim
