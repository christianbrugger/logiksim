#include "render_generic.h"

#include "exception.h"
#include "geometry/orientation.h"
#include "geometry/scene.h"
#include "render/context_info.h"

#include <blend2d.h>
#include <gsl/gsl>

#include <array>
#include <cmath>
#include <type_traits>

namespace logicsim {

//
// Layer Surface
//

auto LayerSurface::initialize(const RenderSettings& new_settings) -> void {
    const auto new_size = new_settings.view_config.size();

    // image size changed
    if (ctx.bl_image.size() != new_size) {
        ctx.end();
        ctx.settings = new_settings;
        ctx.bl_image = BLImage {new_size.w, new_size.h, BL_FORMAT_PRGB32};
        ctx.begin();
    }

    // context info changed
    else if (!equals(context_info(ctx.settings), context_info(new_settings))) {
        ctx.end();
        ctx.settings = new_settings;
        ctx.begin();
    }

    // only settings update
    else {
        ctx.settings = new_settings;
    }
}

auto LayerSurface::clear() -> void {
    ctx.clear();
}

auto LayerSurface::shrink_to_fit() -> void {
    ctx.shrink_to_fit();
}

auto render_to_layer(Context& target_ctx, LayerSurface& surface, BLRectI dirty_rect,
                     std::function<void(Context&, bool)> render_func) -> void {
    auto _ [[maybe_unused]] = make_context_guard(target_ctx);

    if (surface.enabled) {
        surface.initialize(target_ctx.settings);
        surface.ctx.bl_ctx.clearRect(dirty_rect);

        {
            auto __ [[maybe_unused]] = make_context_guard(surface);
            render_func(surface.ctx, surface.enabled);
        }

        surface.ctx.sync();
        target_ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_OVER);
        target_ctx.bl_ctx.blitImage(dirty_rect, surface.ctx.bl_image, dirty_rect);
    } else {
        render_func(target_ctx, surface.enabled);
    }
}

[[nodiscard]] auto make_context_guard(LayerSurface& surface) -> ContextGuard {
    return make_context_guard(surface.ctx);
}

//
// Strokes
//

auto resolve_stroke_width(int attribute, const ViewConfig& view_config) -> int {
    return attribute == defaults::use_view_config_stroke_width
               ? view_config.stroke_width()
               : attribute;
}

auto resolve_stroke_width(int attribute, const Context& ctx) -> int {
    return resolve_stroke_width(attribute, ctx.settings.view_config);
}

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

template <>
auto format(PointShape type) -> std::string {
    switch (type) {
        using enum PointShape;

        case circle:
            return "circle";
        case full_circle:
            return "full_circle";
        case cross:
            return "cross";
        case plus:
            return "plus";
        case square:
            return "square";
        case full_square:
            return "full_square";
        case diamond:
            return "diamond";
        case horizontal:
            return "horizontal";
        case vertical:
            return "vertical";
    }

    throw_exception("cannot convert PointType to string");
}

auto draw_point(Context& ctx, point_t point, PointShape shape, color_t color,
                grid_fine_t size) -> void {
    constexpr auto stroke_width = 1;

    switch (shape) {
        using enum PointShape;

        case circle: {
            const auto center = to_context(point, ctx);
            const auto r = to_context(size, ctx);

            ctx.bl_ctx.setStrokeWidth(stroke_width);
            ctx.bl_ctx.strokeCircle(BLCircle {center.x, center.y, r}, color);
            return;
        }
        case full_circle: {
            const auto center = to_context(point, ctx);
            const auto r = to_context(size, ctx);

            ctx.bl_ctx.fillCircle(BLCircle {center.x, center.y, r}, color);
            return;
        }
        case cross: {
            const auto [x, y] = to_context(point, ctx);
            const auto d = to_context(size, ctx);

            ctx.bl_ctx.setStrokeWidth(stroke_width);
            ctx.bl_ctx.strokeLine(BLLine {x - d, y - d, x + d, y + d}, color);
            ctx.bl_ctx.strokeLine(BLLine {x - d, y + d, x + d, y - d}, color);
            return;
        }
        case plus: {
            const auto [x, y] = to_context(point, ctx);
            const auto d = to_context(size, ctx);
            const auto attrs = LineAttributes {color, stroke_width};

            draw_orthogonal_line(ctx, BLLine {x, y + d, x, y - d}, attrs);
            draw_orthogonal_line(ctx, BLLine {x - d, y, x + d, y}, attrs);
            return;
        }
        case square: {
            draw_rect(ctx,
                      rect_fine_t {
                          point_fine_t {point.x - size, point.y - size},
                          point_fine_t {point.x + size, point.y + size},
                      },
                      RectAttributes {
                          .draw_type = ShapeDrawType::stroke,
                          .stroke_width = stroke_width,
                          .stroke_color = color,
                      });

            return;
        }
        case full_square: {
            draw_rect(ctx,
                      rect_fine_t {
                          point_fine_t {point.x - size, point.y - size},
                          point_fine_t {point.x + size, point.y + size},
                      },
                      RectAttributes {
                          .draw_type = ShapeDrawType::fill,
                          .stroke_width = stroke_width,
                          .fill_color = color,
                      });
            return;
        }
        case diamond: {
            const auto [x, y] = to_context(point, ctx);
            const auto d = to_context(size, ctx);

            const auto poly = std::array {BLPoint {x, y - d}, BLPoint {x + d, y},
                                          BLPoint {x, y + d}, BLPoint {x - d, y}};
            const auto view = BLArrayView<BLPoint> {poly.data(), poly.size()};

            ctx.bl_ctx.setStrokeWidth(stroke_width);
            ctx.bl_ctx.strokePolygon(BLArrayView<BLPoint>(view), color);
            return;
        }
        case horizontal: {
            const auto [x, y] = to_context(point, ctx);
            const auto d = to_context(size, ctx);
            const auto attrs = LineAttributes {color, stroke_width};

            draw_orthogonal_line(ctx, BLLine {x - d, y, x + d, y}, attrs);
            return;
        }
        case vertical: {
            const auto [x, y] = to_context(point, ctx);
            const auto d = to_context(size, ctx);
            const auto attrs = LineAttributes {color, stroke_width};

            draw_orthogonal_line(ctx, BLLine {x, y + d, x, y - d}, attrs);
            return;
        }
    }

    throw_exception("unknown shape type.");
}

//
// Arrow
//

auto draw_arrow(Context& ctx, point_t point, color_t color, orientation_t orientation,
                grid_fine_t size) -> void {
    auto _ [[maybe_unused]] = make_context_guard(ctx);

    ctx.bl_ctx.setStrokeWidth(1);
    ctx.bl_ctx.setStrokeStyle(color);

    const auto [x, y] = to_context(point, ctx);
    const auto d = to_context(size, ctx);
    const auto angle = to_angle(orientation);

    ctx.bl_ctx.translate(BLPoint {x, y});
    ctx.bl_ctx.rotate(angle);

    ctx.bl_ctx.strokeLine(BLLine(0, 0, d, 0));
    ctx.bl_ctx.strokeLine(BLLine(0, 0, d * 0.5, +d * 0.25));
    ctx.bl_ctx.strokeLine(BLLine(0, 0, d * 0.5, -d * 0.25));
}

//
// Line
//

auto _draw_orthogonal_line_ordered(Context& ctx, const BLLine line,
                                   LineAttributes attributes) -> void {
    assert(line.x0 <= line.x1);
    assert(line.y0 <= line.y1);

    const int stroke_width = resolve_stroke_width(attributes.stroke_width, ctx);

    if (stroke_width < 1) {
        return;
    }

    const int offset = (stroke_width - 1) / 2;

    const int p0_cap = attributes.p0_endcap ? offset : 0;
    const int p1_cap = attributes.p1_endcap ? stroke_width - offset : 0;

    if (line.y0 == line.y1) {
        const auto x = line.x0 - p0_cap;
        const auto w = line.x1 + p1_cap - x;
        ctx.bl_ctx.fillRect(x, line.y0 - offset, w, stroke_width, attributes.color);
    }

    else {
        const auto y = line.y0 - p0_cap;
        const auto h = line.y1 + p1_cap - y;
        ctx.bl_ctx.fillRect(line.x0 - offset, y, stroke_width, h, attributes.color);
    }
}

auto draw_orthogonal_line(Context& ctx, BLLine line, LineAttributes attributes) -> void {
    if (line.x0 > line.x1) {
        std::swap(line.x0, line.x1);
        std::swap(attributes.p0_endcap, attributes.p1_endcap);

    } else if (line.y0 > line.y1) {
        std::swap(line.y0, line.y1);
        std::swap(attributes.p0_endcap, attributes.p1_endcap);
    }

    _draw_orthogonal_line_ordered(ctx, line, attributes);
}

auto draw_line(Context& ctx, const ordered_line_t line, LineAttributes attributes)
    -> void {
    draw_line(ctx, line_fine_t {line}, attributes);
}

auto draw_line(Context& ctx, const line_t line, LineAttributes attributes) -> void {
    draw_line(ctx, line_fine_t {line}, attributes);
}

auto draw_line(Context& ctx, const line_fine_t line, LineAttributes attributes) -> void {
    const auto [x0, y0] = to_context(line.p0, ctx);
    const auto [x1, y1] = to_context(line.p1, ctx);

    draw_orthogonal_line(ctx, BLLine {x0, y0, x1, y1}, attributes);
}

//
// Rect
//

auto _draw_rect_stroke(Context& ctx, rect_fine_t rect, RectAttributes attributes)
    -> void {
    const auto [x0, y0] = to_context(rect.p0, ctx);
    const auto [x1, y1] = to_context(rect.p1, ctx);

    const auto w = std::max(1., x1 - x0);
    const auto h = std::max(1., y1 - y0);

    const auto width = resolve_stroke_width(attributes.stroke_width, ctx);

    ctx.bl_ctx.setStrokeWidth(width);
    ctx.bl_ctx.strokeRect(x0 + width / 2.0, y0 + width / 2.0, w - width, h - width,
                          attributes.stroke_color);
}

auto _draw_rect_fill(Context& ctx, rect_fine_t rect, RectAttributes attributes) -> void {
    const auto [x0, y0] = to_context(rect.p0, ctx);
    const auto [x1, y1] = to_context(rect.p1, ctx);

    const auto w = std::max(1., x1 - x0);
    const auto h = std::max(1., y1 - y0);

    ctx.bl_ctx.fillRect(x0, y0, w, h, attributes.fill_color);
}

auto _draw_rect_fill_and_stroke(Context& ctx, rect_fine_t rect,
                                RectAttributes attributes) {
    const auto stroke_width = resolve_stroke_width(attributes.stroke_width, ctx);

    auto [x0, y0] = to_context(rect.p0, ctx);
    auto [x1, y1] = to_context(rect.p1, ctx);

    auto w = std::max(1., x1 - x0);
    auto h = std::max(1., y1 - y0);

    if (stroke_width > 0) {
        ctx.bl_ctx.fillRect(x0, y0, w, h, attributes.stroke_color);

        x0 += stroke_width;
        y0 += stroke_width;
        w -= stroke_width * 2;
        h -= stroke_width * 2;
    }

    if (w >= 1 && h >= 1) {
        ctx.bl_ctx.fillRect(x0, y0, w, h, attributes.fill_color);
    }
}

auto draw_rect(Context& ctx, rect_fine_t rect, RectAttributes attributes) -> void {
    switch (attributes.draw_type) {
        case ShapeDrawType::fill:
            return _draw_rect_fill(ctx, rect, attributes);
        case ShapeDrawType::stroke:
            return _draw_rect_stroke(ctx, rect, attributes);
        case ShapeDrawType::fill_and_stroke:
            return _draw_rect_fill_and_stroke(ctx, rect, attributes);
    }

    throw_exception("unknown ShapeDrawType in draw_rect");
}

auto draw_round_rect(Context& ctx, rect_fine_t rect, RoundRectAttributes attributes)
    -> void {
    const auto&& [x0, y0] = to_context(rect.p0, ctx);
    const auto&& [x1, y1] = to_context(rect.p1, ctx);

    auto w = x1 - x0;
    auto h = y1 - y0;

    if (attributes.draw_type == ShapeDrawType::fill) {
        ++w;
        ++h;
    }

    w = w == 0 ? 1.0 : w;
    h = h == 0 ? 1.0 : h;

    const auto r = attributes.rounding == defaults::maximum_rounding
                       ? std::min(w, h) / 2.0
                       : to_context(attributes.rounding, ctx);

    if (do_fill(attributes.draw_type)) {
        ctx.bl_ctx.fillRoundRect(BLRoundRect {x0, y0, w, h, r}, attributes.fill_color);
    }

    if (do_stroke(attributes.draw_type)) {
        const auto width = resolve_stroke_width(attributes.stroke_width, ctx);
        const auto offset = stroke_offset(width);

        ctx.bl_ctx.setStrokeWidth(width);
        ctx.bl_ctx.strokeRoundRect(BLRoundRect {x0 + offset, y0 + offset, w, h, r},
                                   attributes.stroke_color);
    }
}

//
// Circle
//

auto _draw_circle_fill_and_stroke(Context& ctx, point_fine_t center, grid_fine_t radius,
                                  CircleAttributes attributes) -> void {
    const auto&& [x0, y0] =
        to_context(point_fine_t {center.x - radius, center.y - radius}, ctx);
    const auto&& [x1, y1] =
        to_context(point_fine_t {center.x + radius, center.y + radius}, ctx);

    const auto x = (x0 + x1) / 2;
    const auto y = (y0 + y1) / 2;

    const auto rx = (x1 - x0) / 2;
    const auto ry = (y1 - y0) / 2;

    const auto stroke_width = resolve_stroke_width(attributes.stroke_width, ctx);

    ctx.bl_ctx.fillEllipse(BLEllipse {x, y, rx, ry}, attributes.stroke_color);
    ctx.bl_ctx.fillEllipse(BLEllipse {x, y, rx - stroke_width, ry - stroke_width},
                           attributes.fill_color);
}

auto _draw_circle_fill(Context& ctx, point_fine_t center, grid_fine_t radius,
                       CircleAttributes attributes) -> void {
    static_cast<void>(ctx);
    static_cast<void>(center);
    static_cast<void>(radius);
    static_cast<void>(attributes);

    throw_exception("not implemented");
}

auto _draw_circle_stroke(Context& ctx, point_fine_t center, grid_fine_t radius,
                         CircleAttributes attributes) -> void {
    static_cast<void>(ctx);
    static_cast<void>(center);
    static_cast<void>(radius);
    static_cast<void>(attributes);

    throw_exception("not implemented");
}

auto draw_circle(Context& ctx, point_fine_t center, grid_fine_t radius,
                 CircleAttributes attributes) -> void {
    switch (attributes.draw_type) {
        using enum ShapeDrawType;

        case fill_and_stroke:
            _draw_circle_fill_and_stroke(ctx, center, radius, attributes);
            break;
        case fill:
            _draw_circle_fill(ctx, center, radius, attributes);
            break;
        case stroke:
            _draw_circle_stroke(ctx, center, radius, attributes);
            break;
    }
}

//
// Text
//

auto draw_text(Context& ctx, point_fine_t position, std::string_view text,
               TextAttributes attributes) -> void {
    if (text.empty()) {
        return;
    }
    const auto font_size_px = to_context_unrounded(attributes.font_size, ctx);
    if (font_size_px < attributes.cuttoff_size_px) {
        return;
    }

    const auto position_px = to_context(position, ctx);
    ctx.text_cache.draw_text(ctx.bl_ctx, position_px, text, font_size_px,
                             attributes.color, attributes.horizontal_alignment,
                             attributes.vertical_alignment, attributes.style);
}

auto draw_icon(Context& ctx, point_fine_t position, icon_t icon,
               IconAttributes attributes) -> void {
    const auto position_px = to_context(position, ctx);
    const auto icon_height_px = to_context_unrounded(attributes.icon_height, ctx);

    ctx.svg_cache.draw_icon(ctx.bl_ctx,
                            SVGCache::IconAttributes {
                                .icon = icon,
                                .position = position_px,
                                .height = icon_height_px,
                                .color = attributes.color,
                                .horizontal_alignment = attributes.horizontal_alignment,
                                .vertical_alignment = attributes.vertical_alignment,
                            });
}

}  // namespace logicsim