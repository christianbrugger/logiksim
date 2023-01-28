
#include "render_scene.h"

#include "algorithm.h"
#include "format.h"
#include "range.h"
#include "timer.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include <algorithm>
#include <utility>

namespace logicsim {

SimulationScene::SimulationScene(const Simulation& simulation) noexcept
    : simulation_ {&simulation},
      draw_data_vector_(simulation.circuit().element_count()) {}

auto SimulationScene::set_position(Circuit::ConstElement element, point2d_t position)
    -> void {
    get_data(element).position = position;
}

auto SimulationScene::set_line_tree(Circuit::ConstElement element,
                                    std::vector<point2d_t> points,
                                    std::vector<wire_index_t> indices) -> void {
    // this method is potentially very slow
    get_data(element).points.assign(points.begin(), points.end());
    get_data(element).indices.assign(indices.begin(), indices.end());
}

auto SimulationScene::get_data(Circuit::ConstElement element) -> DrawData& {
    return draw_data_vector_.at(element.element_id());
}

auto SimulationScene::get_data(Circuit::ConstElement element) const -> const DrawData& {
    return draw_data_vector_.at(element.element_id());
}

auto SimulationScene::draw_background(BLContext& ctx) const -> void {
    ctx.setFillStyle(BLRgba32(0xFFFFFFFFu));
    ctx.fillAll();
}

auto interpolate_1d(grid_t v0, grid_t v1, double ratio) -> double {
    return v0 + (v1 - v0) * ratio;
}

auto interpolate_line_1d(point2d_t p0, point2d_t p1, time_t t_start, time_t t_end,
                         time_t t_select) -> point2d_fine_t {
    assert(t_start <= t_end);

    if (t_select <= t_start) {
        return static_cast<point2d_fine_t>(p0);
    }
    if (t_select >= t_end) {
        return static_cast<point2d_fine_t>(p1);
    }
    // TODO check numerics
    const double ratio
        = static_cast<double>((t_select - t_start).count()) / (t_end - t_start).count();
    if (p0.x == p1.x) {
        return point2d_fine_t {static_cast<double>(p0.x),
                               interpolate_1d(p0.y, p1.y, ratio)};
    }
    assert(p0.y == p1.y);
    return point2d_fine_t {interpolate_1d(p0.x, p1.x, ratio), static_cast<double>(p0.y)};
}

auto stroke_line_fast(BLContext& ctx, const BLLine& line, BLRgba32 color) -> void {
    auto& image = *ctx.targetImage();
    BLImageData data {};
    auto res = image.getData(&data);

    if (res != BL_SUCCESS) [[unlikely]] {
        throw_exception("could not get image data");
    }
    if (data.format != BL_FORMAT_PRGB32) [[unlikely]] {
        throw_exception("unsupported format");
    }

    auto* array = static_cast<uint32_t*>(data.pixelData);
    if (line.x0 == line.x1) {
        auto y0 = static_cast<int>(std::round(line.y0));
        auto y1 = static_cast<int>(std::round(line.y1));

        if (y0 > y1) {
            auto y2 = y0;
            y0 = y1;
            y1 = y2;
        }

        for (auto y : range(y0, y1 + 1)) {
            array[static_cast<int>(std::round(line.x0)) + image.width() * y]
                = color.value;
        }
    } else {
        auto x0 = static_cast<int>(std::round(line.x0));
        auto x1 = static_cast<int>(std::round(line.x1));

        if (x0 > x1) {
            auto x2 = x0;
            x0 = x1;
            x1 = x2;
        }

        for (auto x : range(x0, x1 + 1)) {
            array[x + image.width() * static_cast<int>(std::round(line.y0))]
                = color.value;
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

auto draw_line_segment(BLContext& ctx, point2d_t p0, point2d_t p1, time_t time_start,
                       time_t time_end, const Simulation::history_vector_t& history,
                       bool wire_enabled) -> void {
    if (history.size() == 0 || time_end >= history.at(0)) {
        draw_line_segment(ctx, p0, p1, wire_enabled);
        return;
    }

    const auto it_start = std::lower_bound(history.rbegin(), history.rend(), time_start);
    const auto it_end = std::lower_bound(history.rbegin(), history.rend(), time_end);

    const auto idx_start = history.rend() - it_start;
    const auto idx_end = history.rend() - it_end;

    if (idx_start == idx_end) {
        const bool value = static_cast<bool>(idx_start % 2) ^ wire_enabled;
        draw_line_segment(ctx, p0, p1, value);
        return;
    }

    auto p_pivot = static_cast<point2d_fine_t>(p0);

    for (auto index : range(idx_start, idx_end + 1)) {
        point2d_fine_t p_end;
        if (index >= std::ssize(history) || time_end >= history.at(index)) {
            p_end = static_cast<point2d_fine_t>(p1);
        } else {
            auto t_end = history.at(index);
            // TODO fix flipping of time_end and time_start
            p_end = interpolate_line_1d(p1, p0, time_end, time_start, t_end);
        }

        const bool value = static_cast<bool>(index % 2) ^ wire_enabled;
        draw_line_segment(ctx, p_pivot, p_end, value);

        p_pivot = p_end;
    }
}

auto SimulationScene::draw_wire(BLContext& ctx, Circuit::ConstElement element) const
    -> void {
    ctx.setStrokeWidth(1);

    const auto& data = get_data(element);

    if (data.points.size() < 2) {
        return;
    }

    const auto& history = simulation_->get_input_history(element);
    const bool wire_enabled = simulation_->input_value(element.input(0));

    auto d0 = delay_t {0us};
    auto p0 = data.points.at(0);
    auto p1 = data.points.at(1);
    size_t i = 2;

    // TODO remove allocation
    folly::small_vector<delay_t, 128> delay_index {};
    delay_index.push_back(d0);

    while (true) {
        auto d1 = delay_t::runtime(
            d0.value + distance_1d(p0, p1) * Simulation::wire_delay_per_distance.value);
        delay_index.push_back(d1);

        const auto time_start = time_t {simulation_->time() - d0.value};
        const auto time_end = time_t {simulation_->time() - d1.value};
        draw_line_segment(ctx, p0, p1, time_start, time_end, history, wire_enabled);

        if (i >= data.points.size()) {
            break;
        }
        auto p0_index = data.indices.at(i - 2);
        d0 = delay_index.at(p0_index);
        p0 = data.points.at(p0_index);
        p1 = data.points.at(i);
        ++i;
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

    // draw inputs & outputs
    // auto input_offset = (height - 1) / 2;  // ranges::size(input_values)) / 2;
    auto input_offset = (height - std::ssize(input_values)) / 2;
    for (int i = 0; const auto value : input_values) {
        double y_pin = y + (input_offset + i) * s;
        uint32_t color = value ? 0xFFFF0000u : 0xFF000000u;
        ctx.setStrokeStyle(BLRgba32(color));
        ctx.strokeLine(BLLine(x, y_pin, x - 0.75 * s, y_pin));
        ++i;
    }

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

    int min_line_points {1};
    int max_line_points {5};

    connection_size_t n_outputs_min {1};
    connection_size_t n_outputs_max {5};

    int min_event_spacing_us {5};
    int max_event_spacing_us {30};
};

template <std::uniform_random_bit_generator G>
auto random_segment_value(grid_t last, const RenderBenchmarkConfig& config, G& rng) {
    auto grid_dist = boost::random::uniform_int_distribution<grid_t> {
        std::max(config.min_grid, gsl::narrow<grid_t>(last - config.max_segment_length)),
        std::min(config.max_grid, gsl::narrow<grid_t>(last + config.max_segment_length))};
    grid_t res;
    while ((res = grid_dist(rng)) == last) {
    }
    return res;
}

template <std::uniform_random_bit_generator G>
auto random_line_point(point2d_t previous, const RenderBenchmarkConfig& config, G& rng)
    -> point2d_t {
    auto orientation_dist = boost::random::uniform_int_distribution<int> {0, 1};

    if (orientation_dist(rng)) {
        return point2d_t {previous.x, random_segment_value(previous.y, config, rng)};
    }
    return point2d_t {random_segment_value(previous.x, config, rng), previous.y};
}

template <std::uniform_random_bit_generator G>
auto random_line_point(point2d_t p0, point2d_t p1, const RenderBenchmarkConfig& config,
                       G& rng) -> point2d_t {
    if (p0.x == p1.x) {
        return point2d_t {random_segment_value(p1.x, config, rng), p1.y};
    }
    return point2d_t {p1.x, random_segment_value(p1.y, config, rng)};
}

// create single line of length n from start_index
auto add_random_line(std::vector<point2d_t>& points, std::vector<wire_index_t>& indices,
                     int n_points, wire_index_t start_index, auto get_next_point) {
    auto index = start_index;

    for (auto _ [[maybe_unused]] : range(n_points)) {
        const auto& p0 = points.at(indices.at(index - 1));
        const auto& p1 = points.at(index);

        points.push_back(get_next_point(p0, p1));
        indices.push_back(index);

        index = gsl::narrow<wire_index_t>(std::ssize(points) - 1);
    }
}

auto is_line_tree_valid(const std::vector<point2d_t>& points) -> bool {
    return !has_duplicates_quadratic(points);
}

template <std::uniform_random_bit_generator G>
auto create_random_line_tree(connection_size_t n_outputs,
                             const RenderBenchmarkConfig& config, G& rng)
    -> std::tuple<std::vector<point2d_t>, std::vector<wire_index_t>,
                  std::vector<wire_index_t>> {
    while (true) {
        auto grid_dist = boost::random::uniform_int_distribution<grid_t> {
            config.min_grid, config.max_grid};
        auto p0 = point2d_t {grid_dist(rng), grid_dist(rng)};

        auto points = std::vector<point2d_t> {p0, random_line_point(p0, config, rng)};
        auto indices = std::vector<wire_index_t> {0};
        auto output_indices = std::vector<wire_index_t> {};

        auto get_next_point = [&](point2d_t p0_, point2d_t p1_) {
            return random_line_point(p0_, p1_, config, rng);
        };

        for (auto i : range(n_outputs)) {
            auto length_dist = boost::random::uniform_int_distribution<int> {
                config.min_line_points, config.max_line_points};
            auto n_points = length_dist(rng);

            wire_index_t start_index;
            if (i == 0) {
                start_index = 1;
            } else {
                auto max_index = gsl::narrow<wire_index_t>(std::ssize(points) - 2);
                auto index_dist = boost::random::uniform_int_distribution<wire_index_t> {
                    1, max_index};
                start_index = index_dist(rng);
            }

            add_random_line(points, indices, n_points, start_index, get_next_point);
            output_indices.push_back(gsl::narrow<wire_index_t>(std::size(points) - 1));
        }

        if (!is_line_tree_valid(points)) {
            continue;
        }

        // TODO decide first index entry
        indices.erase(indices.begin());

        return {points, indices, output_indices};
    }
}

auto calculate_delay(const std::vector<point2d_t>& points,
                     const std::vector<wire_index_t>& indices, wire_index_t output_index)
    -> delay_t {
    auto delay = delay_t {0us};

    auto p1_index = output_index;
    while (p1_index > 0) {
        auto p0_index = (p1_index >= 2) ? indices.at(p1_index - 2) : wire_index_t {0};

        auto& p1 = points.at(p1_index);
        auto& p0 = points.at(p0_index);

        auto segment_delay
            = distance_1d(p0, p1) * Simulation::wire_delay_per_distance.value;
        delay = delay_t::runtime(delay.value + segment_delay);
        p1_index = p0_index;
    }

    return delay;
}

auto calculate_tree_length(std::vector<point2d_t> points,
                           std::vector<wire_index_t> indices) -> int {
    int res = distance_1d(points.at(0), points.at(1));
    int i = 2;
    for (auto index : indices) {
        res += distance_1d(points.at(index), points.at(i));
        ++i;
    }
    return res;
}

auto fill_line_scene(BenchmarkScene& scene, int n_lines) -> int64_t {
    auto rng = boost::random::mt19937 {0};
    const auto config = RenderBenchmarkConfig {};
    auto tree_length_sum = int64_t {0};

    // create scene
    auto& circuit = scene.circuit;
    // auto circuit = Circuit {};
    for (auto _ [[maybe_unused]] : range(n_lines)) {
        boost::random::uniform_int_distribution<connection_size_t> output_dist {
            config.n_outputs_min, config.n_outputs_max};
        circuit.add_element(ElementType::wire, 1, output_dist(rng));
    }
    add_output_placeholders(circuit);

    auto& simulation = scene.simulation = Simulation {circuit};
    auto& renderer = scene.renderer = SimulationScene {simulation};
    // auto simulation = Simulation {circuit};
    // auto renderer = SimulationScene {simulation};

    // add line trees
    for (auto element : circuit.elements()) {
        if (element.element_type() == ElementType::wire) {
            auto [points, indices, output_indices]
                = create_random_line_tree(element.output_count(), config, rng);
            renderer.set_line_tree(element, points, indices);

            // set delays
            auto delays = std::vector<delay_t> {};
            for (auto output_index : output_indices) {
                auto delay = calculate_delay(points, indices, output_index);

                auto output
                    = element.output(gsl::narrow<connection_size_t>(std::size(delays)));
                simulation.set_output_delay(output, delay);

                delays.push_back(delay);
            }

            auto tree_max_delay = std::ranges::max(delays);
            simulation.set_max_history(element, history_t::runtime(tree_max_delay.value));

            tree_length_sum += calculate_tree_length(points, indices);
        }
    }

    // initialize simulation
    simulation.initialize();

    // get maximum delay
    delay_t max_delay {0us};
    for (auto element : circuit.elements()) {
        for (auto output : element.outputs()) {
            if (simulation.output_delay(output) > max_delay) {
                max_delay = simulation.output_delay(output);
            }
        }
    }
    time_t max_time {max_delay.value};
    // add events
    for (auto element : circuit.elements()) {
        if (element.element_type() == ElementType::wire) {
            auto spacing_dist_us = boost::random::uniform_int_distribution<int> {
                config.min_event_spacing_us, config.max_event_spacing_us};
            bool next_value = true;
            time_t next_time = time_t {spacing_dist_us(rng) * 1us};

            while (next_time < max_time) {
                simulation.submit_event(element.input(0), next_time, next_value);

                next_value = next_value ^ true;
                next_time = next_time + spacing_dist_us(rng) * 1us;
            }
        }
    }

    // run simulation
    {
        // auto timer = Timer {"Simulation", Timer::Unit::ms, 3};
        simulation.run(max_time);
    }

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
