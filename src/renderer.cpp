
#include "renderer.h"

#include "algorithm.h"
#include "format.h"
#include "layout_calculations.h"
#include "range.h"
#include "timer.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include <algorithm>
#include <array>
#include <numbers>
#include <numeric>
#include <utility>

namespace logicsim {

auto stroke_width(const RenderSettings& settings) -> int {
    constexpr static auto stepping = 12;
    const auto scale = settings.view_config.scale;

    return std::max(1, static_cast<int>(scale / stepping));
}

auto line_cross_width(const RenderSettings& settings) -> int {
    constexpr static auto stepping = 8;
    const auto scale = settings.view_config.scale;

    return std::max(1, static_cast<int>(scale / stepping));
}

auto stroke_offset(const RenderSettings& settings) -> double {
    // To allign our strokes to the pixel grid, we need to offset odd strokes
    // otherwise they are drawn between pixels and get blurry
    if (stroke_width(settings) % 2 == 0) {
        return 0;
    }
    return 0.5;
}

class new_context {
   public:
    explicit new_context(BLContext& ctx) : ctx_(ctx) {
        ctx_.save();
    }

    ~new_context() {
        ctx_.restore();
    };

   private:
    BLContext& ctx_;
};

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

// TODO rename
auto draw_connector_fast(BLContext& ctx, const point_t point, bool enabled, int width,
                         const RenderSettings& settings) -> void {
    const uint32_t color = enabled ? 0xFFFF0000u : 0xFF000000u;

    // TODO refactor getting data & width
    BLImageData data = get_image_data(ctx);
    auto& image = *ctx.targetImage();
    auto* array = static_cast<uint32_t*>(data.pixelData);

    const auto w = image.width();
    const auto h = image.height();

    const auto p_ctx = to_context(point, settings.view_config);

    const auto x = static_cast<int>(p_ctx.x);
    const auto y = static_cast<int>(p_ctx.y);

    const int s = width;
    for (int xi : range(x - s, x + s + 1)) {
        for (int yj : range(y - s, y + s + 1)) {
            if (xi >= 0 && xi < w && yj >= 0 && yj < h) {
                array[xi + w * yj] = color;
            }
        }
    }
}

// TODO rename
auto draw_connector_blend2d(BLContext& ctx, const point_t point, bool enabled, int width,
                            const RenderSettings& settings) {
    if (width < 1) {
        return;
    }

    const auto p_ctx = to_context(point, settings.view_config);
    const int half = width;
    const int size = 2 * half + 1;

    const uint32_t color = enabled ? 0xFFFF0000u : 0xFF000000u;

    ctx.setFillStyle(BLRgba32 {color});
    ctx.fillRect(p_ctx.x - half, p_ctx.y - half, size, size);
}

auto stroke_line_fast(BLContext& ctx, const BLLine& line, BLRgba32 color) -> void {
    // TODO refactor getting data & width
    BLImageData data = get_image_data(ctx);
    auto& image = *ctx.targetImage();
    auto* array = static_cast<uint32_t*>(data.pixelData);

    const auto w = image.width();
    const auto h = image.height();

    if (line.x0 == line.x1) {
        auto x = static_cast<int>(round_fast(line.x0));
        auto y0 = static_cast<int>(round_fast(line.y0));
        auto y1 = static_cast<int>(round_fast(line.y1));

        if (y0 > y1) {
            std::swap(y0, y1);
        }

        for (auto y : range(y0, y1 + 1)) {
            if (x >= 0 && x < w && y >= 0 && y < h) {
                array[x + w * y] = color.value;
            }
        }
    } else {
        auto x0 = static_cast<int>(round_fast(line.x0));
        auto x1 = static_cast<int>(round_fast(line.x1));
        auto y = static_cast<int>(round_fast(line.y0));

        if (x0 > x1) {
            std::swap(x0, x1);
        }

        for (auto x : range(x0, x1 + 1)) {
            if (x >= 0 && x < w && y >= 0 && y < h) {
                array[x + w * y] = color.value;
            }
        }
    }
}

auto stroke_line_blend2d(BLContext& ctx, const BLLine& line, BLRgba32 color, int width)
    -> void {
    if (width < 1) {
        return;
    }
    ctx.setFillStyle(color);

    const auto offset = (width - 1) / 2;

    if (line.y0 == line.y1) {
        auto x0 = line.x0;
        auto x1 = line.x1;

        if (x0 > x1) {
            std::swap(x0, x1);
        }

        auto w = x1 - x0 + 1;

        ctx.fillRect(x0, line.y0 - offset, w, width);
    } else {
        auto y0 = line.y0;
        auto y1 = line.y1;

        if (y0 > y1) {
            std::swap(y0, y1);
        }

        auto h = y1 - y0 + 1;

        ctx.fillRect(line.x0 - offset, y0, width, h);
    }
}

auto stroke_line_blend2d_2(BLContext& ctx, const BLLine& line, BLRgba32 color) -> void {
    ctx.setStrokeStyle(color);
    ctx.setStrokeWidth(1.0);

    if (line.y0 == line.y1) {
        if (line.x1 > line.x0) {
            ctx.strokeLine(line.x0, line.y0 + 0.5, line.x1 + 1.0, line.y1 + 0.5);
        } else {
            ctx.strokeLine(line.x0 + 1.0, line.y0 + 0.5, line.x1, line.y1 + 0.5);
        }
    } else {
        if (line.y1 > line.y0) {
            ctx.strokeLine(line.x0 + 0.5, line.y0, line.x1 + 0.5, line.y1 + 1.0);
        } else {
            ctx.strokeLine(line.x0 + 0.5, line.y0 + 1.0, line.x1 + 0.5, line.y1);
        }
    }
}

// TODO rename
auto draw_connector_impl(BLContext& ctx, const point_t point, bool enabled, int width,
                         const RenderSettings& settings) {
    // draw_connector_fast(ctx, point, enabled, width, settings);
    draw_connector_blend2d(ctx, point, enabled, width, settings);
}

auto stroke_line_impl(BLContext& ctx, const BLLine& line, BLRgba32 color, int width)
    -> void {
    // stroke_line_fast(ctx, line, color);
    stroke_line_blend2d(ctx, line, color, width);
    // stroke_line_blend2d_2(ctx, line, color);
}

template <typename PointType>
auto draw_line_segment(BLContext& ctx, PointType p0, PointType p1, bool wire_enabled,
                       const RenderSettings& settings) -> void {
    const uint32_t color = wire_enabled ? 0xFFFF0000u : 0xFF000000u;

    const auto [x0, y0] = to_context(p0, settings.view_config);
    const auto [x1, y1] = to_context(p1, settings.view_config);

    const auto width = stroke_width(settings);

    stroke_line_impl(ctx, BLLine(x0, y0, x1, y1), BLRgba32(color), width);
}

auto draw_line_segment(BLContext& ctx, point_t p_from, point_t p_until, time_t time_from,
                       time_t time_until, const Simulation::HistoryView& history,
                       const RenderSettings& settings) -> void {
    assert(time_from < time_until);

    const auto it_from = history.from(time_from);
    const auto it_until = history.until(time_until);

    for (const auto& entry : std::ranges::subrange(it_from, it_until)) {
        const auto p_start = interpolate_line_1d(p_from, p_until, time_from, time_until,
                                                 entry.first_time);
        const auto p_end = interpolate_line_1d(p_from, p_until, time_from, time_until,
                                               entry.last_time);
        draw_line_segment(ctx, p_start, p_end, entry.value, settings);
    }
}

auto draw_wire(BLContext& ctx, Schematic::ConstElement element, const Layout& layout,
               const RenderSettings& settings) -> void {
    const auto lc_width = line_cross_width(settings);

    for (auto&& segment : layout.line_tree(element).sized_segments()) {
        draw_line_segment(ctx, segment.line.p1, segment.line.p0, false, settings);

        if (segment.has_connector_p0) {
            draw_connector_impl(ctx, segment.line.p0, false, lc_width, settings);
        }
    }
}

auto draw_wire(BLContext& ctx, Schematic::ConstElement element, const Layout& layout,
               const Simulation& simulation, const RenderSettings& settings) -> void {
    const auto lc_width = line_cross_width(settings);

    // TODO move to some class
    const auto to_time = [time = simulation.time()](LineTree::length_t length_) {
        return time_t {time.value
                       - static_cast<int64_t>(length_)
                             * Simulation::wire_delay_per_distance.value};
    };

    const auto history = simulation.input_history(element);

    for (auto&& segment : layout.line_tree(element).sized_segments()) {
        draw_line_segment(ctx, segment.line.p1, segment.line.p0,
                          to_time(segment.p1_length), to_time(segment.p0_length), history,
                          settings);

        if (segment.has_connector_p0) {
            bool wire_enabled = history.value(to_time(segment.p0_length));
            draw_connector_impl(ctx, segment.line.p0, wire_enabled, lc_width, settings);
        }
    }
}

auto draw_single_connector(BLContext& ctx, point_t position, orientation_t orientation,
                           bool enabled, const RenderSettings& settings) -> void {
    const auto endpoint = connector_endpoint(position, orientation);

    const auto p0 = to_context(position, settings.view_config);
    const auto p1 = to_context(endpoint, settings.view_config);

    // TODO put this in function
    const uint32_t color = enabled ? 0xFFFF0000u : 0xFF000000u;

    // const auto stroke = stroke_width(settings);
    // const auto offset = stroke_offset(settings);

    // ctx.setStrokeStyle(BLRgba32(color));
    // ctx.setStrokeWidth(stroke);

    // const auto line = [=]() {
    //     if (orientation == orientation_t::left || orientation == orientation_t::right)
    //     {
    //         return BLLine(p0.x, p0.y + offset, p1.x, p1.y + offset);
    //     }
    //     return BLLine(p0.x + offset, p0.y, p1.x + offset, p1.y);
    // }();

    const auto width = stroke_width(settings);

    stroke_line_impl(ctx, BLLine {p0.x, p0.y, p1.x, p1.y}, BLRgba32(color), width);
}

auto draw_connectors(BLContext& ctx, Schematic::ConstElement element,
                     const Layout& layout, const Simulation* simulation,
                     const RenderSettings& settings) -> void {
    const auto layout_data
        = to_layout_calculation_data(element.schematic(), layout, element.element_id());

    if (simulation == nullptr) {
        iter_input_location(
            layout_data, [&](point_t position, orientation_t orientation) {
                draw_single_connector(ctx, position, orientation, false, settings);
                return true;
            });

        iter_output_location(
            layout_data, [&](point_t position, orientation_t orientation) {
                draw_single_connector(ctx, position, orientation, false, settings);
                return true;
            });
    } else {
        iter_input_location_and_id(
            layout_data,
            [&](connection_id_t input_id, point_t position, orientation_t orientation) {
                const auto enabled = simulation->input_value(element.input(input_id));
                draw_single_connector(ctx, position, orientation, enabled, settings);
                return true;
            });

        iter_output_location_and_id(
            layout_data,
            [&](connection_id_t output_id, point_t position, orientation_t orientation) {
                const auto enabled = simulation->output_value(element.output(output_id));
                draw_single_connector(ctx, position, orientation, enabled, settings);
                return true;
            });
    }
}

auto draw_standard_element(BLContext& ctx, Schematic::ConstElement element,
                           const Layout& layout, const Simulation* simulation,
                           const RenderSettings& settings) -> void {
    const auto position = layout.position(element);
    const auto element_height = std::max(element.input_count(), element.output_count());

    const auto extra_space = 0.4;
    const auto offset = stroke_offset(settings);

    ctx.setStrokeWidth(stroke_width(settings));
    ctx.setFillStyle(BLRgba32(0xFFFFFF80u));
    ctx.setStrokeStyle(BLRgba32(0xFF000000u));

    const auto [x0, y0] = to_context(
        point_fine_t {position.x.value * 1.0, position.y.value - extra_space},
        settings.view_config);

    const auto [x1, y1]
        = to_context(point_fine_t {position.x.value + 2.0,
                                   position.y.value + extra_space + element_height - 1},
                     settings.view_config);

    const auto w_ = x1 - x0;
    const auto h_ = y1 - y0;

    const auto w = w_ == 0 ? 1.0 : w_;
    const auto h = h_ == 0 ? 1.0 : h_;

    ctx.fillRect(x0, y0, w, h);
    ctx.strokeRect(x0 + offset, y0 + offset, w, h);

    draw_connectors(ctx, element, layout, simulation, settings);
}

auto render_circuit(BLContext& ctx, const Schematic& schematic, const Layout& layout,
                    const Simulation* simulation, const RenderSettings& settings)
    -> void {
    for (auto element : schematic.elements()) {
        auto type = element.element_type();

        if (type == ElementType::wire) {
            if (simulation == nullptr) {
                draw_wire(ctx, element, layout, settings);
            } else {
                draw_wire(ctx, element, layout, *simulation, settings);
            }
        } else if (type != ElementType::placeholder) {
            draw_standard_element(ctx, element, layout, simulation, settings);
        }
    }
}

auto draw_grid_space_limit(BLContext& ctx, const RenderSettings& settings) {
    const auto p0
        = to_context(point_t {grid_t::min(), grid_t::min()}, settings.view_config);
    const auto p1
        = to_context(point_t {grid_t::max(), grid_t::max()}, settings.view_config);

    ctx.setStrokeStyle(BLRgba32(0xFF808080u));
    ctx.setStrokeWidth(std::max(5.0, to_context(5.0, settings.view_config)));
    ctx.strokeRect(p0.x + 0.5, p0.y + 0.5, p1.x - p0.x, p1.y - p0.y);
}

constexpr auto monochrome(uint8_t value) -> BLRgba32 {
    return BLRgba32 {0xFF000000u + value * 0x1u + value * 0x100u + value * 0x10000u};
}

auto draw_background_pattern(BLContext& ctx, const RenderSettings& settings) {
    //  TODO put in render settings
    const auto min_grid_gap = 10;  // pixel

    const auto a0 = to_grid_fine(0, 0, settings.view_config);
    const auto a1
        = to_grid_fine(ctx.targetWidth(), ctx.targetHeight(), settings.view_config);

    const auto clamp_to_grid = [](double v_) {
        return gsl::narrow_cast<grid_t::value_type>(
            std::clamp(v_, grid_t::min() * 1.0, grid_t::max() * 1.0));
    };

    constexpr static auto grid_definition = {
        std::tuple {grid_t::value_type {1}, monochrome(0xF0), 1},
        std::tuple {grid_t::value_type {8}, monochrome(0xE4), 1},
        std::tuple {grid_t::value_type {64}, monochrome(0xE4), 2},
        std::tuple {grid_t::value_type {512}, monochrome(0xD8), 2},
        std::tuple {grid_t::value_type {4096}, monochrome(0xC0), 2},
    };

    for (auto&& [delta, color, width] : grid_definition) {
        const auto g0 = point_t {
            clamp_to_grid(std::floor(a0.x / delta) * delta),
            clamp_to_grid(std::floor(a0.y / delta) * delta),
        };
        const auto g1 = point_t {
            clamp_to_grid(std::ceil(a1.x / delta) * delta),
            clamp_to_grid(std::ceil(a1.y / delta) * delta),
        };

        const auto p0 = to_context(g0, settings.view_config);
        const auto p1 = to_context(g1, settings.view_config);

        if (to_context(delta, settings.view_config) >= min_grid_gap) {
            // vertical
            for (int x = g0.x.value; x <= g1.x.value; x += delta) {
                const auto cx = round_fast((x + settings.view_config.offset.x)
                                           * settings.view_config.scale);
                stroke_line_impl(ctx, BLLine {cx, p0.y, cx, p1.y}, color, width);
            }
            // horizontal
            for (int y = g0.y.value; y <= g1.y.value; y += delta) {
                const auto cy = round_fast((y + settings.view_config.offset.y)
                                           * settings.view_config.scale);
                stroke_line_impl(ctx, BLLine {p0.x, cy, p1.x, cy}, color, width);
            }
        }
    }
}

auto render_background(BLContext& ctx, const RenderSettings& settings) -> void {
    ctx.setFillStyle(BLRgba32(0xFFFFFFFFu));
    ctx.fillAll();

    draw_background_pattern(ctx, settings);
    draw_grid_space_limit(ctx, settings);
}

auto render_point(BLContext& ctx, point_t point, PointShape shape, color_t color,
                  double size, const RenderSettings& settings) -> void {
    ctx.setStrokeWidth(1);
    ctx.setStrokeStyle(BLRgba32(color.value));
    ctx.setFillStyle(BLRgba32(color.value));

    switch (shape) {
        using enum PointShape;

        case circle: {
            const auto center = to_context(point, settings.view_config);
            const auto r = size * settings.view_config.scale;

            ctx.strokeCircle(BLCircle {center.x, center.y, r});
            return;
        }
        case cross: {
            const auto [x, y] = to_context(point, settings.view_config);
            const auto d = size * settings.view_config.scale;

            ctx.strokeLine(BLLine {x - d, y - d, x + d, y + d});
            ctx.strokeLine(BLLine {x - d, y + d, x + d, y - d});
            return;
        }
        case plus: {
            const auto [x, y] = to_context(point, settings.view_config);
            const auto d = size * settings.view_config.scale;

            ctx.strokeLine(BLLine {x, y + d, x, y - d});
            ctx.strokeLine(BLLine {x - d, y, x + d, y});
            return;
        }
        case square: {
            const auto [x, y] = to_context(point, settings.view_config);
            const auto d = size * settings.view_config.scale;

            ctx.strokeRect(BLRect {x - d, y - d, 2 * d, 2 * d});
            return;
        }
        case full_square: {
            const auto [x, y] = to_context(point, settings.view_config);
            const auto d = size * settings.view_config.scale;

            ctx.fillRect(BLRect {x - d, y - d, 2 * d, 2 * d});
            return;
        }
        case diamond: {
            const auto [x, y] = to_context(point, settings.view_config);
            const auto d = size * settings.view_config.scale;

            const auto poly = std::array {BLPoint {x, y - d}, BLPoint {x + d, y},
                                          BLPoint {x, y + d}, BLPoint {x - d, y}};
            const auto view = BLArrayView<BLPoint> {poly.data(), poly.size()};
            ctx.strokePolygon(BLArrayView<BLPoint>(view));
            return;
        }
        case horizontal: {
            const auto [x, y] = to_context(point, settings.view_config);
            const auto d = size * settings.view_config.scale;

            ctx.strokeLine(BLLine {x - d, y, x + d, y});
            return;
        }
        case vertical: {
            const auto [x, y] = to_context(point, settings.view_config);
            const auto d = size * settings.view_config.scale;

            ctx.strokeLine(BLLine {x, y + d, x, y - d});
            return;
        }
    }

    throw_exception("unknown shape type.");
}

auto render_arrow(BLContext& ctx, point_t point, color_t color, orientation_t orientation,
                  double size, const RenderSettings& settings) -> void {
    auto _ = new_context {ctx};

    ctx.setStrokeWidth(1);
    ctx.setStrokeStyle(BLRgba32(color.value));

    const auto [x, y] = to_context(point, settings.view_config);
    const auto d = size * settings.view_config.scale;
    const auto angle = to_angle(orientation);

    ctx.translate(BLPoint {x, y});
    ctx.rotate(angle);

    ctx.strokeLine(BLLine(0, 0, d, 0));
    ctx.strokeLine(BLLine(0, 0, d * 0.5, +d * 0.25));
    ctx.strokeLine(BLLine(0, 0, d * 0.5, -d * 0.25));
}

auto render_input_marker(BLContext& ctx, point_t point, color_t color,
                         orientation_t orientation, double size,
                         const RenderSettings& settings) -> void {
    auto _ = new_context {ctx};

    ctx.setStrokeWidth(1);
    ctx.setStrokeStyle(BLRgba32(color.value));

    const auto [x, y] = to_context(point, settings.view_config);
    const auto d = size * settings.view_config.scale;
    const auto angle = to_angle(orientation);

    ctx.translate(BLPoint {x, y});
    ctx.rotate(angle);

    const auto pi = std::numbers::pi;

    ctx.strokeArc(BLArc {0, 0, d, d, -pi / 2, pi});
    ctx.strokeLine(BLLine {-d, -d, 0, -d});
    ctx.strokeLine(BLLine {-d, +d, 0, +d});
}

//
// Editable Circuit
//

auto render_editable_circuit_connection_cache(BLContext& ctx,
                                              const EditableCircuit& editable_circuit,
                                              const RenderSettings& settings) -> void {
    for (auto [position, orientation] :
         editable_circuit.input_positions_and_orientations()) {
        const auto size = 1.0 / 3.0;
        render_input_marker(ctx, position, defaults::color_green, orientation, size,
                            settings);
    }

    for (auto [position, orientation] :
         editable_circuit.output_positions_and_orientations()) {
        const auto size = 0.8;
        render_arrow(ctx, position, defaults::color_green, orientation, size, settings);
    }
}

auto render_editable_circuit_collision_cache(BLContext& ctx,
                                             const EditableCircuit& editable_circuit,
                                             const RenderSettings& settings) -> void {
    fmt::print("Collision states count: {}\n",
               editable_circuit.collision_states().size());

    for (auto [point, state] : editable_circuit.collision_states()) {
        const auto color = defaults::color_orange;
        const auto size = 0.25;

        switch (state) {
            using enum CollisionCache::CollisionState;

            case element_body: {
                render_point(ctx, point, PointShape::square, color, size, settings);
                break;
            }
            case element_connection: {
                render_point(ctx, point, PointShape::circle, color, size, settings);
                break;
            }
            case wire_connection: {
                render_point(ctx, point, PointShape::full_square, color, size * (2. / 3),
                             settings);
                break;
            }
            case wire_horizontal: {
                render_point(ctx, point, PointShape::horizontal, color, size, settings);
                break;
            }
            case wire_vertical: {
                render_point(ctx, point, PointShape::vertical, color, size, settings);
                break;
            }
            case wire_point: {
                render_point(ctx, point, PointShape::diamond, color, size, settings);
                break;
            }
            case wire_crossing: {
                render_point(ctx, point, PointShape::plus, color, size, settings);
                break;
            }
            case element_wire_connection: {
                render_point(ctx, point, PointShape::circle, color, size, settings);
                render_point(ctx, point, PointShape::full_square, color, size * (2. / 3),
                             settings);
                break;
            }
            case invalid_state: {
                throw_exception("invalid state encountered");
                break;
            }
        }
    }
}

auto render_editable_circuit_caches(BLContext& ctx,
                                    const EditableCircuit& editable_circuit,
                                    const RenderSettings& settings) -> void {
    render_editable_circuit_connection_cache(ctx, editable_circuit, settings);
    render_editable_circuit_collision_cache(ctx, editable_circuit, settings);
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
        render_circuit(ctx, scene.schematic, scene.layout, &scene.simulation);
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
