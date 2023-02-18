
#include "renderer.h"

#include "algorithm.h"
#include "format.h"
#include "range.h"
#include "timer.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include <algorithm>
#include <numeric>
#include <utility>

namespace logicsim {

auto interpolate_1d(grid_t v0, grid_t v1, double ratio) -> double {
    return v0.value + (v1.value - v0.value) * ratio;
}

auto interpolate_line_1d(point_t p0, point_t p1, time_t t0, time_t t1, time_t t_select)
    -> point_fine_t {
    assert(t0 < t1);

    if (t_select <= t0) {
        return static_cast<point_fine_t>(p0);
    }
    if (t_select >= t1) {
        return static_cast<point_fine_t>(p1);
    }

    const double alpha = static_cast<double>((t_select.value - t0.value).count())
                         / static_cast<double>((t1.value - t0.value).count());

    if (is_horizontal(line_t {p0, p1})) {
        return point_fine_t {interpolate_1d(p0.x, p1.x, alpha),
                             static_cast<double>(p0.y)};
    }
    return point_fine_t {static_cast<double>(p0.x), interpolate_1d(p0.y, p1.y, alpha)};
}

auto get_image_data(BLContext& ctx) -> BLImageData {
    auto image = ctx.targetImage();
    if (image == nullptr) [[unlikely]] {
        throw_exception("context has no image attached");
    }

    BLImageData data {};
    auto res = image->getData(&data);

    if (res != BL_SUCCESS) [[unlikely]] {
        throw_exception("could not get image data");
    }
    if (data.format != BL_FORMAT_PRGB32) [[unlikely]] {
        throw_exception("unsupported format");
    }
    return data;
}

auto draw_connector_fast(BLContext& ctx, const point_t point, BLRgba32 color) -> void {
    // TODO refactor getting data & width
    BLImageData data = get_image_data(ctx);
    auto& image = *ctx.targetImage();
    auto* array = static_cast<uint32_t*>(data.pixelData);

    int w = image.width();
    int x = point.x.value * 12;
    int y = point.y.value * 12;

    static constexpr int s = 2;
    for (int xi : range(x - s, x + s + 1)) {
        for (int yj : range(y - s, y + s + 1)) {
            array[xi + w * yj] = color.value;
        }
    }
}

auto stroke_line_fast(BLContext& ctx, const BLLine& line, BLRgba32 color) -> void {
    // TODO refactor getting data & width
    BLImageData data = get_image_data(ctx);
    auto& image = *ctx.targetImage();
    auto* array = static_cast<uint32_t*>(data.pixelData);

    if (line.x0 == line.x1) {
        auto x = static_cast<int>(std::round(line.x0));
        auto y0 = static_cast<int>(std::round(line.y0));
        auto y1 = static_cast<int>(std::round(line.y1));

        if (y0 > y1) {
            std::swap(y0, y1);
        }

        int w = image.width();
        for (auto y : range(y0, y1 + 1)) {
            array[x + w * y] = color.value;
        }
    } else {
        auto x0 = static_cast<int>(std::round(line.x0));
        auto x1 = static_cast<int>(std::round(line.x1));
        auto y = static_cast<int>(std::round(line.y0));

        if (x0 > x1) {
            std::swap(x0, x1);
        }

        int w = image.width();
        for (auto x : range(x0, x1 + 1)) {
            array[x + w * y] = color.value;
        }
    }
}

template <typename PointType>
auto draw_line_segment(BLContext& ctx, PointType p0, PointType p1, bool wire_enabled)
    -> void {
    const uint32_t color = wire_enabled ? 0xFFFF0000u : 0xFF000000u;
    constexpr static double s = 12;

    // ctx.setStrokeStyle(BLRgba32(color));
    // ctx.strokeLine(BLLine(p0.x * s, p0.y * s, p1.x * s, p1.y * s));

    stroke_line_fast(ctx, BLLine(p0.x * s, p0.y * s, p1.x * s, p1.y * s),
                     BLRgba32(color));
}

auto draw_line_segment(BLContext& ctx, point_t p_from, point_t p_until, time_t time_from,
                       time_t time_until, const Simulation::HistoryView& history)
    -> void {
    assert(time_from < time_until);

    const auto it_from = history.from(time_from);
    const auto it_until = history.until(time_until);

    for (const auto& entry : std::ranges::subrange(it_from, it_until)) {
        const auto p_start = interpolate_line_1d(p_from, p_until, time_from, time_until,
                                                 entry.first_time);
        const auto p_end = interpolate_line_1d(p_from, p_until, time_from, time_until,
                                               entry.last_time);
        draw_line_segment(ctx, p_start, p_end, entry.value);
    }
}

auto draw_wire(BLContext& ctx, Schematic::ConstElement element, const Layout& layout,
               const Simulation& simulation) -> void {
    // ctx.setStrokeWidth(1);

    // TODO move to some class
    const auto to_time = [time = simulation.time()](LineTree::length_t length_) {
        return time_t {time.value
                       - static_cast<int64_t>(length_)
                             * Simulation::wire_delay_per_distance.value};
    };

    const auto history = simulation.input_history(element);

    for (auto&& segment : layout.line_tree(element).sized_segments()) {
        draw_line_segment(ctx, segment.line.p1, segment.line.p0,
                          to_time(segment.p1_length), to_time(segment.p0_length),
                          history);

        if (segment.has_connector_p0) {
            bool wire_enabled = history.value(to_time(segment.p0_length));
            const uint32_t color = wire_enabled ? 0xFFFF0000u : 0xFF000000u;
            draw_connector_fast(ctx, segment.line.p0, BLRgba32(color));
        }
    }
}

auto draw_standard_element(BLContext& ctx, Schematic::ConstElement element,
                           const Layout& layout, const Simulation& simulation) -> void {
    constexpr static double s = 12;
    ctx.setStrokeWidth(1);

    auto position = layout.position(element);
    auto input_values = simulation.input_values(element);
    auto output_values = simulation.output_values(element);

    // draw rect
    double x = position.x * s;
    double y = position.y * s;
    auto height = std::max(std::ssize(input_values), std::ssize(output_values));
    BLPath path;
    path.addRect(x, y + -0.5 * s, 2 * s, height * s);

    // draw inputs
    auto input_offset = (height - std::ssize(input_values)) / 2;
    for (int i = 0; const auto value : input_values) {
        double y_pin = y + (input_offset + i) * s;
        uint32_t color = value ? 0xFFFF0000u : 0xFF000000u;
        ctx.setStrokeStyle(BLRgba32(color));
        ctx.strokeLine(BLLine(x, y_pin, x - 0.75 * s, y_pin));
        ++i;
    }

    // draw outputs
    auto output_offset = (height - std::ssize(output_values)) / 2;
    for (int i = 0; const auto value : output_values) {
        double y_pin = y + (output_offset + i) * s;
        uint32_t color = value ? 0xFFFF0000u : 0xFF000000u;
        ctx.setStrokeStyle(BLRgba32(color));
        ctx.strokeLine(BLLine(x + 2 * s, y_pin, x + 2.75 * s, y_pin));
        ++i;
    }

    ctx.setFillStyle(BLRgba32(0xFFFFFF00u));
    ctx.setStrokeStyle(BLRgba32(0xFF000000u));
    ctx.fillPath(path);
    ctx.strokePath(path);
}

auto render_circuit(BLContext& ctx, const Layout& layout, const Simulation& simulation,
                    const RenderSettings& settings) -> void {
    // TODO move globally
    ctx.postTranslate(BLPoint(0.5, 0.5));
    ctx.postScale(1);

    for (auto element : simulation.schematic().elements()) {
        auto type = element.element_type();

        if (type == ElementType::wire) {
            draw_wire(ctx, element, layout, simulation);
        } else if (type != ElementType::placeholder) {
            draw_standard_element(ctx, element, layout, simulation);
        }
    }
}

auto render_background(BLContext& ctx, const RenderSettings& settings) -> void {
    ctx.setFillStyle(BLRgba32(0xFFFFFFFFu));
    ctx.fillAll();
}

auto render_point_shape(BLContext& ctx, point_t point, PointShape shape, double size,
                        const RenderSettings& settings) -> void {
    switch (shape) {
        using enum PointShape;

        case circle: {
            const auto cx = point.x * settings.scale;
            const auto cy = point.y * settings.scale;
            const auto r = size;

            ctx.strokeCircle(BLCircle {cx, cy, r});
            return;
        }
        case cross: {
            const auto x = point.x * settings.scale;
            const auto y = point.y * settings.scale;
            const auto d = size;

            ctx.strokeLine(BLLine {x - d, y - d, x + d, y + d});
            ctx.strokeLine(BLLine {x - d, y + d, x + d, y - d});
            return;
        }
        case plus: {
            const auto x = point.x * settings.scale;
            const auto y = point.y * settings.scale;
            const auto d = size;

            ctx.strokeLine(BLLine {x, y + d, x, y - d});
            ctx.strokeLine(BLLine {x - d, y, x + d, y});
            return;
        }
        case square: {
            const auto x = point.x * settings.scale;
            const auto y = point.y * settings.scale;
            const auto d = size;

            ctx.strokeLine(BLLine {x - d, y + d, x - d, y - d});
            ctx.strokeLine(BLLine {x + d, y + d, x + d, y - d});

            ctx.strokeLine(BLLine {x - d, y + d, x + d, y + d});
            ctx.strokeLine(BLLine {x - d, y - d, x + d, y - d});
            return;
        }
        case diamond: {
            const auto x = point.x * settings.scale;
            const auto y = point.y * settings.scale;
            const auto d = size;

            ctx.strokeLine(BLLine {x, y + d, x + d, y});
            ctx.strokeLine(BLLine {x, y + d, x - d, y});

            ctx.strokeLine(BLLine {x, y - d, x + d, y});
            ctx.strokeLine(BLLine {x, y - d, x - d, y});
            return;
        }
        case horizontal: {
            const auto x = point.x * settings.scale;
            const auto y = point.y * settings.scale;
            const auto d = size;

            ctx.strokeLine(BLLine {x - d, y, x + d, y});
            return;
        }
        case vertical: {
            const auto x = point.x * settings.scale;
            const auto y = point.y * settings.scale;
            const auto d = size;

            ctx.strokeLine(BLLine {x, y + d, x, y - d});
            return;
        }
    }

    throw_exception("unknown shape type.");
}

namespace detail {

auto set_point_style(BLContext& ctx, color_t color) -> void {
    ctx.setStrokeWidth(1);
    ctx.setStrokeStyle(BLRgba32(color.value));
}

}  // namespace detail

auto render_point(BLContext& ctx, point_t point, PointShape shape, color_t color,
                  double size, const RenderSettings& settings) -> void {
    detail::set_point_style(ctx, color);
    render_point_shape(ctx, point, shape, size, settings);
}

//
// Editable Circuit
//

auto render_editable_circuit_caches(BLContext& ctx,
                                    const EditableCircuit& editable_circuit,
                                    const RenderSettings& settings) -> void {
    // connection caches
    {
        const auto size = settings.scale * (1.0 / 3.0);
        render_points(ctx, editable_circuit.input_positions(), PointShape::circle,
                      defaults::color_green, size, settings);
        render_points(ctx, editable_circuit.output_positions(), PointShape::cross,
                      defaults::color_green, size, settings);
    }

    // collision cache
    for (auto [point, state] : editable_circuit.collision_states()) {
        const auto color = defaults::color_orange;
        const auto size = settings.scale * (1.0 / 4.0);

        switch (state) {
            using enum CollisionCache::CollisionState;

            case element_body: {
                render_point(ctx, point, PointShape::square, color, size);
                break;
            }
            case wire_horizontal: {
                render_point(ctx, point, PointShape::horizontal, color, size);
                break;
            }
            case wire_vertical: {
                render_point(ctx, point, PointShape::vertical, color, size);
                break;
            }
            case wire_point: {
                render_point(ctx, point, PointShape::diamond, color, size);
                break;
            }
            case wire_crossing: {
                render_point(ctx, point, PointShape::plus, color, size);
                break;
            }
            case body_and_wire: {
                render_point(ctx, point, PointShape::circle, color, size);
                break;
            }
        }
    }
}

//
// benchmark
//

struct RenderBenchmarkConfig {
    grid_t min_grid {1};
    grid_t max_grid {99};

    grid_t max_segment_length {5};

    int min_line_segments {1};
    int max_line_segments {5};

    int n_outputs_min {1};
    int n_outputs_max {5};

    int min_event_spacing_us {5};
    int max_event_spacing_us {30};
};

namespace {

template <typename T>
using UDist = boost::random::uniform_int_distribution<T>;

template <std::uniform_random_bit_generator G>
auto get_udist(grid_t a, grid_t b, G& rng) {
    return [a, b, &rng]() -> grid_t {
        return grid_t {UDist<grid_t::value_type> {a.value, b.value}(rng)};
    };
}

template <std::uniform_random_bit_generator G>
auto random_segment_value(grid_t last, const RenderBenchmarkConfig& config, G& rng) {
    // auto grid_dist = boost::random::uniform_int_distribution<grid_t> {
    //     std::max(config.min_grid, last - config.max_segment_length),
    //     std::min(config.max_grid, last + config.max_segment_length)};

    auto grid_dist
        = get_udist(std::max(config.min_grid, last - config.max_segment_length),
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
auto pick_line_point(line_t line, G& rng) -> point_t {
    line = order_points(line);
    // return point2d_t {UDist<grid_t> {line.p0.x, line.p1.x}(rng),
    //                   UDist<grid_t> {line.p0.y, line.p1.y}(rng)};
    return point_t {get_udist(line.p0.x, line.p1.x, rng)(),
                    get_udist(line.p0.y, line.p1.y, rng)()};
}

template <std::uniform_random_bit_generator G>
auto create_line_tree_segment(point_t start_point, bool horizontal,
                              const RenderBenchmarkConfig& config, G& rng) -> LineTree {
    auto segment_count_dist
        = UDist<int> {config.min_line_segments, config.max_line_segments};
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

    const auto is_horizontal = UDist<int> {0, 1}(rng);
    return create_line_tree_segment(p0, is_horizontal, config, rng);
}

template <std::uniform_random_bit_generator G>
auto create_random_line_tree(std::size_t n_outputs, const RenderBenchmarkConfig& config,
                             G& rng) -> LineTree {
    auto line_tree = create_first_line_tree_segment(config, rng);

    while (line_tree.output_count() < n_outputs) {
        auto new_tree = std::optional<LineTree> {};
        // TODO flatten loop
        do {
            const auto segment_index = UDist<int> {0, line_tree.segment_count() - 1}(rng);
            const auto segment = line_tree.segment(segment_index);
            const auto origin = pick_line_point(segment, rng);

            const auto sub_tree
                = create_line_tree_segment(origin, is_vertical(segment), config, rng);
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
    auto rng = boost::random::mt19937 {0};
    const auto config = RenderBenchmarkConfig {};
    auto tree_length_sum = int64_t {0};

    // create schematics
    auto& schematic = scene.schematic;
    for (auto _ [[maybe_unused]] : range(n_lines)) {
        UDist<int> output_dist {config.n_outputs_min, config.n_outputs_max};
        schematic.add_element(ElementType::wire, 1, output_dist(rng));
    }
    add_output_placeholders(schematic);

    // create layout
    auto& layout = scene.layout = Layout {};
    for (auto _ [[maybe_unused]] : range(schematic.element_count())) {
        layout.add_default_element();
    }

    // add line trees
    auto& simulation = scene.simulation = Simulation {schematic};
    for (auto element : schematic.elements()) {
        if (element.element_type() == ElementType::wire) {
            auto line_tree = create_random_line_tree(element.output_count(), config, rng);

            // delays
            auto lengths = line_tree.calculate_output_lengths();
            assert(lengths.size() == element.output_count());
            auto delays
                = transform_to_vector(lengths, [](LineTree::length_t length) -> delay_t {
                      return delay_t {Simulation::wire_delay_per_distance.value * length};
                  });
            simulation.set_output_delays(element, delays);

            // history
            auto tree_max_delay = std::ranges::max(delays);
            simulation.set_history_length(element, delay_t {tree_max_delay.value});

            tree_length_sum += calculate_tree_length(line_tree);
            layout.set_line_tree(element, std::move(line_tree));
        }
    }

    // init simulation
    simulation.initialize();

    // calculate simulation time
    delay_t max_delay {0us};
    for (auto element : schematic.elements()) {
        for (auto output : element.outputs()) {
            max_delay = std::max(max_delay, simulation.output_delay(output));
        }
    }
    auto max_time {max_delay.value};

    // add events
    for (auto element : schematic.elements()) {
        if (element.element_type() == ElementType::wire) {
            auto spacing_dist_us
                = UDist<int> {config.min_event_spacing_us, config.max_event_spacing_us};
            bool next_value = true;
            auto next_time = spacing_dist_us(rng) * 1us;

            while (next_time < max_time) {
                simulation.submit_event(element.input(connection_id_t {0}), next_time,
                                        next_value);

                next_value = next_value ^ true;
                next_time = next_time + spacing_dist_us(rng) * 1us;
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
    BLImage img(1200, 1200, BL_FORMAT_PRGB32);
    BLContext ctx(img);
    render_background(ctx);
    {
        auto timer = Timer {"Render", Timer::Unit::ms, 3};
        render_circuit(ctx, scene.layout, scene.simulation);
    }
    ctx.end();

    if (save_image) {
        BLImageCodec codec;
        codec.findByName("PNG");
        img.writeToFile("benchmark_line_renderer.png", codec);
    }

    return tree_length_sum;
}

}  // namespace logicsim
