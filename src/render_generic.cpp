#include "render_generic.h"

#include "exception.h"
#include "geometry.h"

#include <blend2d.h>

#include <array>

namespace logicsim {

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

auto LayerSurface::is_initialized(const ViewConfig& config) const -> bool {
    return image.width() == config.width() || image.height() == config.height();
}

auto LayerSurface::initialize(const ViewConfig& config, const BLContextCreateInfo& info)
    -> void {
    if (!is_initialized(config)) {
        image = BLImage {config.width(), config.height(), BL_FORMAT_PRGB32};
        ctx.begin(image, info);
    }
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

auto render_to_layer(BLContext& target_ctx, LayerSurface& layer, BLRectI dirty_rect,
                     const RenderSettings& settings,
                     std::function<void(BLContext&)> render_func) -> void {
    target_ctx.save();

    if (layer.enabled) {
        layer.initialize(settings.view_config, context_info(settings));
        layer.ctx.clearRect(dirty_rect);

        {
            layer.ctx.save();
            render_func(layer.ctx);
            layer.ctx.restore();
        }

        checked_sync(layer.ctx);
        target_ctx.setCompOp(BL_COMP_OP_SRC_OVER);
        target_ctx.blitImage(dirty_rect, layer.image, dirty_rect);
    } else {
        render_func(target_ctx);
    }

    target_ctx.restore();
}

//
// Layers Cache
//

auto LayersCache::format() const -> std::string {
    return fmt::format(
        "LayersCache("
        "\n  normal_below = {}"
        "\n  normal_wires = {}"
        "\n  normal_above = {}"
        "\n"
        "\n  uninserted_below = {}"
        "\n  uninserted_above = {}"
        "\n"
        "\n  selected_logic_items = {}"
        "\n  selected_wires = {}"
        "\n  temporary_wires = {}"
        "\n  valid_logic_items = {}"
        "\n  valid_wires = {}"
        "\n  colliding_logic_items = {}"
        "\n  colliding_wires = {}"
        "\n"
        "\n  uninserted_bounding_rect = {}"
        "\n  overlay_bounding_rect = {}"
        "\n)",

        normal_below,  //
        normal_wires,  //
        normal_above,  //

        uninserted_below,  //
        uninserted_above,  //

        selected_logic_items,   //
        selected_wires,         //
        temporary_wires,        //
        valid_logic_items,      //
        valid_wires,            //
        colliding_logic_items,  //
        colliding_wires,        //

        uninserted_bounding_rect,  //
        overlay_bounding_rect      //
    );
}

auto LayersCache::clear() -> void {
    normal_below.clear();
    normal_wires.clear();
    normal_above.clear();

    uninserted_below.clear();
    uninserted_above.clear();

    selected_logic_items.clear();
    selected_wires.clear();
    temporary_wires.clear();
    valid_logic_items.clear();
    valid_wires.clear();
    colliding_logic_items.clear();
    colliding_wires.clear();

    uninserted_bounding_rect.reset();
    overlay_bounding_rect.reset();
}

auto LayersCache::has_inserted() const -> bool {
    return !normal_below.empty() ||  //
           !normal_wires.empty() ||  //
           !normal_above.empty();
}

auto LayersCache::has_uninserted() const -> bool {
    return !uninserted_below.empty() ||  //
           !temporary_wires.empty() ||   //
           !colliding_wires.empty() ||   //
           !uninserted_above.empty();
}

auto LayersCache::has_overlay() const -> bool {
    return !selected_logic_items.empty() ||   //
           !selected_wires.empty() ||         //
           !temporary_wires.empty() ||        //
           !valid_logic_items.empty() ||      //
           !valid_wires.empty() ||            //
           !colliding_logic_items.empty() ||  //
           !colliding_wires.empty();
}

auto LayersCache::calculate_overlay_bounding_rect() -> void {
    const auto update = [this](ordered_line_t line) { update_overlay_rect(*this, line); };
    const auto update_info = [this](segment_info_t info) {
        update_overlay_rect(*this, info.line);
    };

    std::ranges::for_each(selected_wires, update);
    std::ranges::for_each(temporary_wires, update_info);
    std::ranges::for_each(valid_wires, update);
    std::ranges::for_each(colliding_wires, update_info);
}

auto update_bounding_rect(std::optional<rect_t>& target, rect_t new_rect) -> void {
    if (!target) {
        target = new_rect;
    } else {
        *target = enclosing_rect(*target, new_rect);
    }
}

auto update_bounding_rect(std::optional<rect_t>& target, ordered_line_t new_line)
    -> void {
    if (!target) {
        target = rect_t {new_line.p0, new_line.p1};
    } else {
        *target = enclosing_rect(*target, new_line);
    }
}

auto update_uninserted_rect(LayersCache& layers, rect_t bounding_rect) -> void {
    update_bounding_rect(layers.uninserted_bounding_rect, bounding_rect);
}

auto update_uninserted_rect(LayersCache& layers, ordered_line_t line) -> void {
    update_bounding_rect(layers.uninserted_bounding_rect, line);
}

auto update_overlay_rect(LayersCache& layers, rect_t bounding_rect) -> void {
    update_bounding_rect(layers.overlay_bounding_rect, bounding_rect);
}

auto update_overlay_rect(LayersCache& layers, ordered_line_t line) -> void {
    update_bounding_rect(layers.overlay_bounding_rect, line);
}

//
// Simulation Layers Cache
//

auto SimulationLayersCache::format() const -> std::string {
    return fmt::format(
        "LayersCache("
        "\n  items_below = {}"
        "\n  wires = {}"
        "\n  items_above = {}"
        "\n)",

        items_below,  //
        wires,        //
        items_above   //
    );
}

auto SimulationLayersCache::clear() -> void {
    items_below.clear();
    wires.clear();
    items_above.clear();
}

//
// RenderSettings
//

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
    ctx.setFillStyle(defaults::color_black);
}

//
// Context Guard
//

ContextGuard::ContextGuard(BLContext& ctx) : ctx_(ctx) {
    ctx_.save();
}

ContextGuard::~ContextGuard() {
    ctx_.restore();
};

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

auto resolve_stroke_width(int attribute, const RenderSettings& settings) -> int {
    return attribute == defaults::use_view_config_stroke_width
               ? settings.view_config.stroke_width()
               : attribute;
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

auto draw_point(BLContext& ctx, point_t point, PointShape shape, color_t color,
                double size, const RenderSettings& settings) -> void {
    constexpr auto stroke_width = 1;

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
            const auto attrs = LineAttributes {color, stroke_width};

            draw_orthogonal_line(ctx, BLLine {x, y + d, x, y - d}, attrs, settings);
            draw_orthogonal_line(ctx, BLLine {x - d, y, x + d, y}, attrs, settings);
            return;
        }
        case square: {
            draw_rect(ctx,
                      rect_fine_t {
                          point_fine_t {point.x.value - size, point.y.value - size},
                          point_fine_t {point.x.value + size, point.y.value + size},
                      },
                      RectAttributes {
                          .draw_type = DrawType::stroke,
                          .stroke_width = stroke_width,
                          .stroke_color = color,
                      },
                      settings);

            return;
        }
        case full_square: {
            draw_rect(ctx,
                      rect_fine_t {
                          point_fine_t {point.x.value - size, point.y.value - size},
                          point_fine_t {point.x.value + size, point.y.value + size},
                      },
                      RectAttributes {
                          .draw_type = DrawType::fill,
                          .stroke_width = stroke_width,
                          .fill_color = color,
                      },
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
            const auto attrs = LineAttributes {color, stroke_width};

            draw_orthogonal_line(ctx, BLLine {x - d, y, x + d, y}, attrs, settings);
            return;
        }
        case vertical: {
            const auto [x, y] = to_context(point, settings.view_config);
            const auto d = to_context(size, settings.view_config);
            const auto attrs = LineAttributes {color, stroke_width};

            draw_orthogonal_line(ctx, BLLine {x, y + d, x, y - d}, attrs, settings);
            return;
        }
    }

    throw_exception("unknown shape type.");
}

//
// Arrow
//

auto draw_arrow(BLContext& ctx, point_t point, color_t color, orientation_t orientation,
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

auto _draw_orthogonal_line_ordered(BLContext& ctx, const BLLine line,
                                   LineAttributes attributes,
                                   const RenderSettings& settings) -> void {
    assert(line.x0 <= line.x1);
    assert(line.y0 <= line.y1);

    const int stroke_width = resolve_stroke_width(attributes.stroke_width, settings);

    if (stroke_width < 1) {
        return;
    }

    const int offset = (stroke_width - 1) / 2;

    const int p0_cap = attributes.p0_endcap ? offset : 0;
    const int p1_cap = attributes.p1_endcap ? stroke_width - offset : 0;

    if (line.y0 == line.y1) {
        const auto x = line.x0 - p0_cap;
        const auto w = line.x1 + p1_cap - x;
        ctx.fillRect(x, line.y0 - offset, w, stroke_width, attributes.color);
    }

    else {
        const auto y = line.y0 - p0_cap;
        const auto h = line.y1 + p1_cap - y;
        ctx.fillRect(line.x0 - offset, y, stroke_width, h, attributes.color);
    }
}

auto draw_orthogonal_line(BLContext& ctx, BLLine line, LineAttributes attributes,
                          const RenderSettings& settings) -> void {
    if (line.x0 > line.x1) {
        std::swap(line.x0, line.x1);
        std::swap(attributes.p0_endcap, attributes.p1_endcap);

    } else if (line.y0 > line.y1) {
        std::swap(line.y0, line.y1);
        std::swap(attributes.p0_endcap, attributes.p1_endcap);
    }

    _draw_orthogonal_line_ordered(ctx, line, attributes, settings);
}

auto draw_line(BLContext& ctx, const ordered_line_t line, LineAttributes attributes,
               const RenderSettings& settings) -> void {
    draw_line(ctx, line_fine_t {line}, attributes, settings);
}

auto draw_line(BLContext& ctx, const line_t line, LineAttributes attributes,
               const RenderSettings& settings) -> void {
    draw_line(ctx, line_fine_t {line}, attributes, settings);
}

auto draw_line(BLContext& ctx, const line_fine_t line, LineAttributes attributes,
               const RenderSettings& settings) -> void {
    const auto [x0, y0] = to_context(line.p0, settings.view_config);
    const auto [x1, y1] = to_context(line.p1, settings.view_config);

    draw_orthogonal_line(ctx, BLLine {x0, y0, x1, y1}, attributes, settings);
}

//
// Rect
//

auto _draw_rect_stroke(BLContext& ctx, rect_fine_t rect, RectAttributes attributes,
                       const RenderSettings& settings) -> void {
    const auto [x0, y0] = to_context(rect.p0, settings.view_config);
    const auto [x1, y1] = to_context(rect.p1, settings.view_config);

    const auto w = std::max(1., x1 - x0);
    const auto h = std::max(1., y1 - y0);

    const auto width = resolve_stroke_width(attributes.stroke_width, settings);

    ctx.setStrokeWidth(width);
    ctx.strokeRect(x0 + width / 2.0, y0 + width / 2.0, w - width, h - width,
                   attributes.stroke_color);
}

auto _draw_rect_fill(BLContext& ctx, rect_fine_t rect, RectAttributes attributes,
                     const RenderSettings& settings) -> void {
    const auto [x0, y0] = to_context(rect.p0, settings.view_config);
    const auto [x1, y1] = to_context(rect.p1, settings.view_config);

    const auto w = std::max(1., x1 - x0);
    const auto h = std::max(1., y1 - y0);

    ctx.fillRect(x0, y0, w, h, attributes.fill_color);
}

auto _draw_rect_fill_and_stroke(BLContext& ctx, rect_fine_t rect,
                                RectAttributes attributes,
                                const RenderSettings& settings) {
    const auto stroke_width = resolve_stroke_width(attributes.stroke_width, settings);

    auto [x0, y0] = to_context(rect.p0, settings.view_config);
    auto [x1, y1] = to_context(rect.p1, settings.view_config);

    auto w = std::max(1., x1 - x0);
    auto h = std::max(1., y1 - y0);

    if (stroke_width > 0) {
        ctx.fillRect(x0, y0, w, h, attributes.stroke_color);

        x0 += stroke_width;
        y0 += stroke_width;
        w -= stroke_width * 2;
        h -= stroke_width * 2;
    }

    if (w >= 1 && h >= 1) {
        ctx.fillRect(x0, y0, w, h, attributes.fill_color);
    }
}

auto draw_rect(BLContext& ctx, rect_fine_t rect, RectAttributes attributes,
               const RenderSettings& settings) -> void {
    switch (attributes.draw_type) {
        case DrawType::fill:
            return _draw_rect_fill(ctx, rect, attributes, settings);
        case DrawType::stroke:
            return _draw_rect_stroke(ctx, rect, attributes, settings);
        case DrawType::fill_and_stroke:
            return _draw_rect_fill_and_stroke(ctx, rect, attributes, settings);
    }

    throw_exception("unknown DrawType in draw_rect");
}

auto draw_round_rect(BLContext& ctx, rect_fine_t rect, RoundRectAttributes attributes,
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

    const auto r = attributes.rounding == defaults::maximum_rounding
                       ? std::min(w, h) / 2.0
                       : to_context(attributes.rounding, settings.view_config);

    if (attributes.draw_type == DrawType::fill ||
        attributes.draw_type == DrawType::fill_and_stroke) {
        ctx.fillRoundRect(x0, y0, w, h, r);
    }

    if (attributes.draw_type == DrawType::stroke ||
        attributes.draw_type == DrawType::fill_and_stroke) {
        const auto width = resolve_stroke_width(attributes.stroke_width, settings);
        const auto offset = stroke_offset(width);

        ctx.setStrokeWidth(width);
        ctx.strokeRoundRect(x0 + offset, y0 + offset, w, h, r);
    }
}

//
// Text
//

auto draw_text(BLContext& ctx, point_fine_t position, std::string_view text,
               TextAttributes attributes, const RenderSettings& settings) -> void {
    if (text.empty()) {
        return;
    }
    const auto font_size_px = attributes.font_size * settings.view_config.pixel_scale();
    if (font_size_px < attributes.cuttoff_size_px) {
        return;
    }

    const auto position_px = to_context(position, settings.view_config);
    settings.text.draw_text(ctx, position_px, text, font_size_px, attributes.color,
                            attributes.horizontal_alignment,
                            attributes.vertical_alignment, attributes.style);
}

}  // namespace logicsim