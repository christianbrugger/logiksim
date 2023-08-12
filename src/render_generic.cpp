#include "render_generic.h"

#include "exceptions.h"
#include "geometry.h"

#include <blend2d.h>

#include <array>

namespace logicsim {

auto draw_orthogonal_line(BLContext& ctx, const BLLine& line, LineAttributes attributes,
                          const RenderSettings& settings) -> void;

namespace {

auto resolve_stroke_width(int stroke_width, const RenderSettings& settings) -> int {
    return stroke_width == defaults::view_config_width
               ? settings.view_config.stroke_width()
               : stroke_width;
}

}  // namespace

auto LayersCache::format() const -> std::string {
    return fmt::format(
        "LayersCache("
        "\n  normal_below = {}"
        "\n  normal_wires = {}"
        "\n  normal_above = {}"
        "\n"
        "\n  uninserted_below = {}"
        "\n  uninserted_wires = {}"
        "\n  uninserted_above = {}"
        "\n"
        "\n  selected_logic_items = {}"
        "\n  selected_wires = {}"
        "\n  temporary_wires = {}"
        "\n  valid_logic_items = {}"
        "\n  valid_wires = {}"
        "\n  colliding_logic_items = {}"
        "\n  colliding_wires = {}"
        "\n)",

        normal_below,  //
        normal_wires,  //
        normal_above,  //

        uninserted_below,  //
        uninserted_wires,  //
        uninserted_above,  //

        selected_logic_items,   //
        selected_wires,         //
        temporary_wires,        //
        valid_logic_items,      //
        valid_wires,            //
        colliding_logic_items,  //
        colliding_wires         //
    );
}

auto LayersCache::clear() const -> void {
    normal_below.clear();
    normal_wires.clear();
    normal_above.clear();

    uninserted_below.clear();
    uninserted_wires.clear();
    uninserted_above.clear();

    selected_logic_items.clear();
    selected_wires.clear();
    temporary_wires.clear();
    valid_logic_items.clear();
    valid_wires.clear();
    colliding_logic_items.clear();
    colliding_wires.clear();
}

auto Layer::initialize(const ViewConfig& config, const BLContextCreateInfo& info)
    -> void {
    if (image.width() != config.width() || image.height() != config.height()) {
        image = BLImage {config.width(), config.height(), BL_FORMAT_PRGB32};

        ctx.begin(image, info);
    }
}

auto RenderSettings::format() const -> std::string {
    return fmt::format(
        "RenderSettings(\n"
        "  view_config = {},\n"
        "  background_grid_min_distance = {})",
        view_config, background_grid_min_distance);
}

auto context_info(const RenderSettings& settings) -> BLContextCreateInfo {
    auto info = BLContextCreateInfo {};
    info.threadCount = gsl::narrow<decltype(info.threadCount)>(settings.thread_count);
    return info;
}

//
// Blend2d specific
//

ContextGuard::ContextGuard(BLContext& ctx) : ctx_(ctx) {
    ctx_.save();
}

ContextGuard::~ContextGuard() {
    ctx_.restore();
};

//
// Generic Types
//

auto stroke_offset(int stroke_width) -> double {
    // To allign our strokes to the pixel grid, we need to offset odd strokes
    // otherwise they are drawn between pixels and get blurry
    if (stroke_width % 2 == 0) {
        return 0;
    }
    return 0.5;
}

//
// Point
//

auto render_point(BLContext& ctx, point_t point, PointShape shape, color_t color_,
                  double size, const RenderSettings& settings) -> void {
    constexpr auto stroke_width = 1;
    const auto color = BLRgba32(color_.value);

    switch (shape) {
        using enum PointShape;

        case circle: {
            const auto center = to_context(point, settings.view_config);
            const auto r = to_context(size, settings.view_config);

            ctx.setStrokeWidth(stroke_width);
            ctx.setStrokeStyle(color);
            ctx.strokeCircle(BLCircle {center.x, center.y, r});
            return;
        }
        case full_circle: {
            const auto center = to_context(point, settings.view_config);
            const auto r = to_context(size, settings.view_config);

            ctx.setFillStyle(color);
            ctx.fillCircle(BLCircle {center.x, center.y, r});
            return;
        }
        case cross: {
            const auto [x, y] = to_context(point, settings.view_config);
            const auto d = to_context(size, settings.view_config);

            ctx.setStrokeWidth(stroke_width);
            ctx.setStrokeStyle(color);

            ctx.strokeLine(BLLine {x - d, y - d, x + d, y + d});
            ctx.strokeLine(BLLine {x - d, y + d, x + d, y - d});
            return;
        }
        case plus: {
            const auto [x, y] = to_context(point, settings.view_config);
            const auto d = to_context(size, settings.view_config);
            const auto attrs = LineAttributes {color_, stroke_width};

            draw_orthogonal_line(ctx, BLLine {x, y + d, x, y - d}, attrs, settings);
            draw_orthogonal_line(ctx, BLLine {x - d, y, x + d, y}, attrs, settings);
            return;
        }
        case square: {
            ctx.setStrokeStyle(color);
            draw_rect(ctx,
                      rect_fine_t {
                          point_fine_t {point.x.value - size, point.y.value - size},
                          point_fine_t {point.x.value + size, point.y.value + size},
                      },
                      RectAttributes {.draw_type = DrawType::stroke,
                                      .stroke_width = stroke_width},
                      settings);

            return;
        }
        case full_square: {
            ctx.setFillStyle(color);
            draw_rect(ctx,
                      rect_fine_t {
                          point_fine_t {point.x.value - size, point.y.value - size},
                          point_fine_t {point.x.value + size, point.y.value + size},
                      },
                      RectAttributes {.draw_type = DrawType::fill,
                                      .stroke_width = stroke_width},
                      settings);
            return;
        }
        case diamond: {
            const auto [x, y] = to_context(point, settings.view_config);
            const auto d = to_context(size, settings.view_config);

            const auto poly = std::array {BLPoint {x, y - d}, BLPoint {x + d, y},
                                          BLPoint {x, y + d}, BLPoint {x - d, y}};
            const auto view = BLArrayView<BLPoint> {poly.data(), poly.size()};

            ctx.setStrokeWidth(stroke_width);
            ctx.setStrokeStyle(color);
            ctx.strokePolygon(BLArrayView<BLPoint>(view));
            return;
        }
        case horizontal: {
            const auto [x, y] = to_context(point, settings.view_config);
            const auto d = to_context(size, settings.view_config);
            const auto attrs = LineAttributes {color_, stroke_width};

            draw_orthogonal_line(ctx, BLLine {x - d, y, x + d, y}, attrs, settings);
            return;
        }
        case vertical: {
            const auto [x, y] = to_context(point, settings.view_config);
            const auto d = to_context(size, settings.view_config);
            const auto attrs = LineAttributes {color_, stroke_width};

            draw_orthogonal_line(ctx, BLLine {x, y + d, x, y - d}, attrs, settings);
            return;
        }
    }

    throw_exception("unknown shape type.");
}

//
// Arrow
//

auto render_arrow(BLContext& ctx, point_t point, color_t color, orientation_t orientation,
                  double size, const RenderSettings& settings) -> void {
    auto _ = ContextGuard {ctx};

    ctx.setStrokeWidth(1);
    ctx.setStrokeStyle(BLRgba32(color.value));

    const auto [x, y] = to_context(point, settings.view_config);
    const auto d = to_context(size, settings.view_config);
    const auto angle = to_angle(orientation);

    ctx.translate(BLPoint {x, y});
    ctx.rotate(angle);

    ctx.strokeLine(BLLine(0, 0, d, 0));
    ctx.strokeLine(BLLine(0, 0, d * 0.5, +d * 0.25));
    ctx.strokeLine(BLLine(0, 0, d * 0.5, -d * 0.25));
}

//
// Line
//

auto draw_orthogonal_line(BLContext& ctx, const BLLine& line, LineAttributes attributes,
                          const RenderSettings& settings) -> void {
    const auto width = resolve_stroke_width(attributes.stroke_width, settings);

    if (width < 1) {
        return;
    }
    ctx.setFillStyle(BLRgba32 {attributes.color.value});

    const int offset = (width - 1) / 2;

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

auto draw_line(BLContext& ctx, const ordered_line_t& line, LineAttributes attributes,
               const RenderSettings& settings) -> void {
    draw_line(ctx, line_fine_t {line}, attributes, settings);
}

auto draw_line(BLContext& ctx, const line_t& line, LineAttributes attributes,
               const RenderSettings& settings) -> void {
    draw_line(ctx, line_fine_t {line}, attributes, settings);
}

auto draw_line(BLContext& ctx, const line_fine_t& line, LineAttributes attributes,
               const RenderSettings& settings) -> void {
    const auto [x0, y0] = to_context(line.p0, settings.view_config);
    const auto [x1, y1] = to_context(line.p1, settings.view_config);

    draw_orthogonal_line(ctx, BLLine(x0, y0, x1, y1), attributes, settings);
}

//
// Rect
//

auto draw_rect(BLContext& ctx, rect_fine_t rect, RectAttributes attributes,
               const RenderSettings& settings) -> void {
    const auto&& [x0, y0] = to_context(rect.p0, settings.view_config);
    const auto&& [x1, y1] = to_context(rect.p1, settings.view_config);

    auto w = x1 - x0;
    auto h = y1 - y0;

    if (attributes.draw_type == DrawType::fill) {
        ++w;
        ++h;
    }

    w = w == 0 ? 1.0 : w;
    h = h == 0 ? 1.0 : h;

    if (attributes.draw_type == DrawType::fill ||
        attributes.draw_type == DrawType::fill_and_stroke) {
        ctx.fillRect(x0, y0, w, h);
    }

    if (attributes.draw_type == DrawType::stroke ||
        attributes.draw_type == DrawType::fill_and_stroke) {
        const auto width = resolve_stroke_width(attributes.stroke_width, settings);
        const auto offset = stroke_offset(width);

        ctx.setStrokeWidth(width);
        ctx.strokeRect(x0 + offset, y0 + offset, w, h);
    }
}

//
// Circuit Primitives
//

auto draw_line_cross_point(BLContext& ctx, const point_t point, bool enabled,
                           const RenderSettings& settings) -> void {
    int lc_width = settings.view_config.line_cross_width();

    if (lc_width < 1) {
        return;
    }

    const int wire_width = settings.view_config.stroke_width();
    const int wire_offset = (wire_width - 1) / 2;

    const int size = 2 * lc_width + wire_width;
    const int offset = wire_offset + lc_width;

    const auto [x, y] = to_context(point, settings.view_config);
    const auto color = enabled ? defaults::color_red : defaults::color_black;

    ctx.setFillStyle(BLRgba32 {color.value});
    ctx.fillRect(x - offset, y - offset, size, size);
}

auto draw_line_segment(BLContext& ctx, line_t line, bool enabled,
                       const RenderSettings& settings) -> void {
    draw_line_segment(ctx, line_fine_t {line}, enabled, settings);

    render_point(ctx, line.p0, PointShape::circle, defaults::color_orange, 0.2, settings);
    render_point(ctx, line.p1, PointShape::cross, defaults::color_orange, 0.2, settings);
}

auto draw_line_segment(BLContext& ctx, line_fine_t line, bool enabled,
                       const RenderSettings& settings) -> void {
    const auto color = enabled ? defaults::color_red : defaults::color_black;
    draw_line(ctx, line, {color}, settings);
}
}  // namespace logicsim