
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

SimulationScene::SimulationScene(const Simulation& simulation) noexcept
    : simulation_ {&simulation},
      draw_data_vector_(simulation.circuit().element_count()) {}

auto SimulationScene::set_position(Circuit::ConstElement element, point2d_t position)
    -> void {
    get_data(element).position = position;
}

auto SimulationScene::set_line_tree(Circuit::ConstElement element, LineTree&& line_tree)
    -> void {
    get_data(element).line_tree = std::move(line_tree);
}

auto SimulationScene::get_data(Circuit::ConstElement element) -> DrawData& {
    return draw_data_vector_.at(element.element_id().value);
}

auto SimulationScene::get_data(Circuit::ConstElement element) const -> const DrawData& {
    return draw_data_vector_.at(element.element_id().value);
}

auto SimulationScene::draw_background(BLContext& ctx) const -> void {
    ctx.setFillStyle(BLRgba32(0xFFFFFFFFu));
    ctx.fillAll();
}

auto interpolate_1d(grid_t v0, grid_t v1, double ratio) -> double {
    return v0.value + (v1.value - v0.value) * ratio;
}

auto interpolate_line_1d(point2d_t p0, point2d_t p1, time_t t0, time_t t1,
                         time_t t_select) -> point2d_fine_t {
    assert(t0 < t1);

    if (t_select <= t0) {
        return static_cast<point2d_fine_t>(p0);
    }
    if (t_select >= t1) {
        return static_cast<point2d_fine_t>(p1);
    }

    const double alpha = static_cast<double>((t_select.value - t0.value).count())
                         / static_cast<double>((t1.value - t0.value).count());

    if (is_horizontal(line2d_t {p0, p1})) {
        return point2d_fine_t {interpolate_1d(p0.x, p1.x, alpha),
                               static_cast<double>(p0.y)};
    }
    return point2d_fine_t {static_cast<double>(p0.x), interpolate_1d(p0.y, p1.y, alpha)};
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

auto draw_connector_fast(BLContext& ctx, const point2d_t point, BLRgba32 color) -> void {
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

auto draw_line_segment(BLContext& ctx, point2d_t p_from, point2d_t p_until,
                       time_t time_from, time_t time_until,
                       const Simulation::HistoryView& history) -> void {
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

auto SimulationScene::draw_wire(BLContext& ctx, Circuit::ConstElement element) const
    -> void {
    // ctx.setStrokeWidth(1);

    // TODO move to some class
    const auto to_time = [time = simulation_->time()](LineTree::length_t length_) {
        return time_t {time.value
                       - static_cast<int64_t>(length_)
                             * Simulation::wire_delay_per_distance.value};
    };

    const auto history = simulation_->input_history(element);

    for (auto&& segment : get_data(element).line_tree.sized_segments()) {
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

auto SimulationScene::draw_standard_element(BLContext& ctx,
                                            Circuit::ConstElement element) const -> void {
    constexpr static double s = 12;
    ctx.setStrokeWidth(1);

    const auto& data = get_data(element);
    auto input_values = simulation_->input_values(element);
    auto output_values = simulation_->output_values(element);

    // draw rect
    double x = data.position.x * s;
    double y = data.position.y * s;
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

auto SimulationScene::render_scene(BLContext& ctx, bool render_background) const -> void {
    ctx.postTranslate(BLPoint(0.5, 0.5));
    ctx.postScale(1);

    if (render_background) {
        draw_background(ctx);
    }

    for (auto element : simulation_->circuit().elements()) {
        auto type = element.element_type();

        if (type == ElementType::wire) {
            draw_wire(ctx, element);
        } else if (type == ElementType::placeholder) {
            ;
        } else {
            draw_standard_element(ctx, element);
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
auto new_line_point(point2d_t origin, bool horizontal,
                    const RenderBenchmarkConfig& config, G& rng) -> point2d_t {
    if (horizontal) {
        return point2d_t {random_segment_value(origin.x, config, rng), origin.y};
    }
    return point2d_t {origin.x, random_segment_value(origin.y, config, rng)};
}

template <std::uniform_random_bit_generator G>
auto new_line_point(point2d_t origin, point2d_t previous,
                    const RenderBenchmarkConfig& config, G& rng) -> point2d_t {
    return new_line_point(origin, is_vertical(line2d_t {previous, origin}), config, rng);
}

// pick random point on line
template <std::uniform_random_bit_generator G>
auto pick_line_point(line2d_t line, G& rng) -> point2d_t {
    line = order_points(line);
    // return point2d_t {UDist<grid_t> {line.p0.x, line.p1.x}(rng),
    //                   UDist<grid_t> {line.p0.y, line.p1.y}(rng)};
    return point2d_t {get_udist(line.p0.x, line.p1.x, rng)(),
                      get_udist(line.p0.y, line.p1.y, rng)()};
}

template <std::uniform_random_bit_generator G>
auto create_line_tree_segment(point2d_t start_point, bool horizontal,
                              const RenderBenchmarkConfig& config, G& rng) -> LineTree {
    auto segment_count_dist
        = UDist<int> {config.min_line_segments, config.max_line_segments};
    auto n_segments = segment_count_dist(rng);

    auto line_tree = std::optional<LineTree> {};
    do {
        auto points = std::vector<point2d_t> {
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
    const auto p0 = point2d_t {grid_dist(), grid_dist()};

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
                                 [](line2d_t line) { return distance_1d(line); });
}

}  // namespace

auto fill_line_scene(BenchmarkScene& scene, int n_lines) -> int64_t {
    auto rng = boost::random::mt19937 {0};
    const auto config = RenderBenchmarkConfig {};
    auto tree_length_sum = int64_t {0};

    // create scene
    auto& circuit = scene.circuit;
    for (auto _ [[maybe_unused]] : range(n_lines)) {
        UDist<int> output_dist {config.n_outputs_min, config.n_outputs_max};
        circuit.add_element(ElementType::wire, 1, output_dist(rng));
    }
    add_output_placeholders(circuit);

    auto& simulation = scene.simulation = Simulation {circuit};
    auto& renderer = scene.renderer = SimulationScene {simulation};

    // add line trees
    for (auto element : circuit.elements()) {
        if (element.element_type() == ElementType::wire) {
            auto line_tree = create_random_line_tree(element.output_count(), config, rng);

            // delays
            auto lengths = line_tree.output_lengths();
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
            renderer.set_line_tree(element, std::move(line_tree));
        }
    }

    // init simulation
    simulation.initialize();

    // calculate simulation time
    delay_t max_delay {0us};
    for (auto element : circuit.elements()) {
        for (auto output : element.outputs()) {
            max_delay = std::max(max_delay, simulation.output_delay(output));
        }
    }
    auto max_time {max_delay.value};

    // add events
    for (auto element : circuit.elements()) {
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
    ctx.setFillStyle(BLRgba32(0xFFFFFFFFu));
    ctx.fillAll();
    {
        auto timer = Timer {"Render", Timer::Unit::ms, 3};
        scene.renderer.render_scene(ctx, false);
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
