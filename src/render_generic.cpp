#include "render_generic.h"

#include "exception.h"
#include "geometry.h"

#include <blend2d.h>

#include <array>
#include <type_traits>

namespace logicsim {

//
// Context
//

auto RenderSettings::format() const -> std::string {
    return fmt::format(
        "RenderSettings(\n"
        "  view_config = {},\n"
        "  background_grid_min_distance = {},\n"
        "  thread_count = {})",
        view_config, background_grid_min_distance, thread_count);
}

auto Context::begin() -> void {
    settings.view_config.set_size(bl_image.width(), bl_image.height());
    bl_ctx.begin(bl_image, context_info(settings));
}

auto Context::sync() -> void {
    checked_sync(bl_ctx);
}

auto Context::end() -> void {
    bl_ctx.end();
    check_errors(bl_ctx);
}

auto Context::clear() -> void {
    text_cache.clear();
    svg_cache.clear();
}

auto Context::shrink_to_fit() -> void {
    text_cache.shrink_to_fit();
    svg_cache.shrink_to_fit();
}

auto equals(const BLContextCreateInfo& a, const BLContextCreateInfo& b) -> bool {
    static_assert(sizeof(BLContextCreateInfo) ==
                  sizeof(BLContextCreateInfo::flags) +
                      sizeof(BLContextCreateInfo::threadCount) +
                      sizeof(BLContextCreateInfo::cpuFeatures) +
                      sizeof(BLContextCreateInfo::commandQueueLimit) +
                      sizeof(BLContextCreateInfo::savedStateLimit) +
                      sizeof(BLContextCreateInfo::pixelOrigin) +
                      sizeof(BLContextCreateInfo::reserved));
    static_assert(std::extent_v<decltype(a.reserved)> == 1);

    return a.flags == b.flags &&                          //
           a.threadCount == b.threadCount &&              //
           a.cpuFeatures == b.cpuFeatures &&              //
           a.commandQueueLimit == b.commandQueueLimit &&  //
           a.savedStateLimit == b.savedStateLimit &&      //
           a.pixelOrigin == b.pixelOrigin &&              //
           a.reserved[0] == b.reserved[0];
}

auto context_info(const RenderSettings& settings) -> BLContextCreateInfo {
    auto info = BLContextCreateInfo {};
    info.commandQueueLimit = 2048;
    info.threadCount = gsl::narrow<decltype(info.threadCount)>(settings.thread_count);
    return info;
}

auto to_context(point_t position, const Context& context) -> BLPoint {
    return to_context(position, context.settings.view_config);
}

auto to_context(point_fine_t position, const Context& context) -> BLPoint {
    return to_context(position, context.settings.view_config);
}

auto to_context(grid_t length, const Context& context) -> double {
    return to_context(length, context.settings.view_config);
}

auto to_context(grid_fine_t length, const Context& context) -> double {
    return to_context(length, context.settings.view_config);
}

//
// Element Draw State
//

template <>
auto format(ElementDrawState state) -> std::string {
    switch (state) {
        using enum ElementDrawState;

        case normal:
            return "normal";
        case normal_selected:
            return "normal_selected";
        case valid:
            return "valid";
        case simulated:
            return "simulated";

        case colliding:
            return "colliding";
        case temporary_selected:
            return "temporary_selected";
    }

    throw_exception("cannot convert ElementDrawState to string");
}

auto is_inserted(ElementDrawState state) noexcept -> bool {
    using enum ElementDrawState;
    return state == normal || state == normal_selected || state == valid ||
           state == simulated;
}

auto has_overlay(ElementDrawState state) noexcept -> bool {
    using enum ElementDrawState;
    return state == normal_selected || state == valid || state == colliding ||
           state == temporary_selected;
}

//
// Drawable Element
//

auto DrawableElement::format() const -> std::string {
    return fmt::format("{}-{}", element_id, state);
}

//
// Layer Surface
//

auto LayerSurface::initialize(const RenderSettings& new_settings) -> void {
    const auto new_width = new_settings.view_config.width();
    const auto new_height = new_settings.view_config.height();

    // image size changed
    if (ctx.bl_image.width() != new_width || ctx.bl_image.height() != new_height) {
        ctx.end();
        ctx.settings = new_settings;
        ctx.bl_image = BLImage {new_width, new_height, BL_FORMAT_PRGB32};
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

auto get_dirty_rect(rect_t bounding_rect, const ViewConfig& view_config) -> BLRectI {
    const auto clamp_x = [&](double x_) {
        return std::clamp(x_, 0., view_config.width() * 1.0);
    };
    const auto clamp_y = [&](double y_) {
        return std::clamp(y_, 0., view_config.height() * 1.0);
    };

    const auto p0 = to_context(bounding_rect.p0, view_config);
    const auto p1 = to_context(bounding_rect.p1, view_config);

    const auto padding = view_config.pixel_scale() * 0.5 + 2;

    const auto x0 = clamp_x(std::trunc(p0.x - padding));
    const auto y0 = clamp_y(std::trunc(p0.y - padding));

    const auto x1 = clamp_x(std::ceil(p1.x + padding + 1));
    const auto y1 = clamp_y(std::ceil(p1.y + padding + 1));

    return BLRectI {
        gsl::narrow<int>(x0),
        gsl::narrow<int>(y0),
        gsl::narrow<int>(x1 - x0),
        gsl::narrow<int>(y1 - y0),
    };
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

//
// Error checks
//

auto check_errors(BLContext& ctx) -> void {
    if (ctx.accumulatedErrorFlags() != BL_CONTEXT_ERROR_NO_FLAGS) [[unlikely]] {
        throw_exception(
            fmt::format("Error in BLContext {}", uint32_t {ctx.accumulatedErrorFlags()})
                .c_str());
    }
}

auto checked_sync(BLContext& ctx) -> void {
    if (ctx.savedStateCount() != 0) {
        throw_exception("context has saved state at sync");
    }

    ctx.flush(BL_CONTEXT_FLUSH_SYNC);
    check_errors(ctx);
}

//
// Context Guard
//

[[nodiscard]] auto make_context_guard(Context& ctx) -> ContextGuard {
    return make_context_guard(ctx.bl_ctx);
}

[[nodiscard]] auto make_context_guard(LayerSurface& surface) -> ContextGuard {
    return make_context_guard(surface.ctx);
}

//
// Draw Type
//

template <>
auto format(DrawType type) -> std::string {
    switch (type) {
        using enum DrawType;

        case fill:
            return "fill";
        case stroke:
            return "stroke";
        case fill_and_stroke:
            return "fill_and_stroke";
    }

    throw_exception("cannot convert DrawType to string");
}

//
// Strokes
//

auto do_fill(DrawType type) -> bool {
    using enum DrawType;
    return type == fill || type == fill_and_stroke;
}

auto do_stroke(DrawType type) -> bool {
    using enum DrawType;
    return type == stroke || type == fill_and_stroke;
}

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
                          .draw_type = DrawType::stroke,
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
                          .draw_type = DrawType::fill,
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
        case DrawType::fill:
            return _draw_rect_fill(ctx, rect, attributes);
        case DrawType::stroke:
            return _draw_rect_stroke(ctx, rect, attributes);
        case DrawType::fill_and_stroke:
            return _draw_rect_fill_and_stroke(ctx, rect, attributes);
    }

    throw_exception("unknown DrawType in draw_rect");
}

auto draw_round_rect(Context& ctx, rect_fine_t rect, RoundRectAttributes attributes)
    -> void {
    const auto&& [x0, y0] = to_context(rect.p0, ctx);
    const auto&& [x1, y1] = to_context(rect.p1, ctx);

    auto w = x1 - x0;
    auto h = y1 - y0;

    if (attributes.draw_type == DrawType::fill) {
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
        using enum DrawType;

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
    const auto font_size_px = to_context(attributes.font_size, ctx);
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
    const auto icon_height_px = to_context(attributes.icon_height, ctx);

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