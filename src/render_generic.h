#ifndef LOGIKSIM_RENDER_GENERIC_H
#define LOGIKSIM_RENDER_GENERIC_H

#include "context_guard.h"
#include "format/enum.h"
#include "format/struct.h"
#include "glyph_cache.h"
#include "svg_cache.h"
#include "vocabulary.h"
#include "vocabulary/font_style.h"
#include "vocabulary/render_setting.h"
#include "vocabulary/shape_draw_type.h"
#include "vocabulary/text_alignment.h"
#include "vocabulary/view_config.h"

#include <blend2d.h>

#include <functional>

namespace logicsim {

struct Context;

namespace defaults {
constexpr inline static auto use_view_config_stroke_width = int {-1};
constexpr inline static auto maximum_rounding = grid_fine_t {-1};
}  // namespace defaults

//
// Context & Settings
//

struct Context {
    BLImage bl_image {};
    BLContext bl_ctx {};
    RenderSettings settings {};
    GlyphCache text_cache {};
    SVGCache svg_cache {};

    auto begin() -> void;
    auto sync() -> void;
    auto end() -> void;

    auto clear() -> void;
    auto shrink_to_fit() -> void;
};

auto equals(const BLContextCreateInfo& a, const BLContextCreateInfo& b) -> bool;

auto context_info(const RenderSettings& settings) -> BLContextCreateInfo;

[[nodiscard]] auto make_context_guard(Context& ctx) -> ContextGuard;

[[nodiscard]] auto to_context(point_t position, const Context& context) -> BLPoint;
[[nodiscard]] auto to_context(point_fine_t position, const Context& context) -> BLPoint;
[[nodiscard]] auto to_context(grid_t length, const Context& context) -> double;
[[nodiscard]] auto to_context(grid_fine_t length, const Context& context) -> double;

[[nodiscard]] auto to_context_unrounded(grid_fine_t length, const Context& context)
    -> double;

//
// Layer Surface
//

struct LayerSurface {
    bool enabled {true};
    Context ctx {};

    auto initialize(const RenderSettings& settings) -> void;

    auto clear() -> void;
    auto shrink_to_fit() -> void;
};

auto get_dirty_rect(rect_t bounding_rect, const ViewConfig& view_config) -> BLRectI;

auto render_to_layer(Context& target_ctx, LayerSurface& surface, BLRectI dirty_rect,
                     std::function<void(Context&, bool)> render_func) -> void;

[[nodiscard]] auto make_context_guard(LayerSurface& surface) -> ContextGuard;

//
// Strokes
//

auto resolve_stroke_width(int attribute, const ViewConfig& view_config) -> int;
auto resolve_stroke_width(int attribute, const Context& ctx) -> int;

auto stroke_offset(int stoke_width) -> double;

//
// Point
//

enum class PointShape {
    circle,
    full_circle,
    cross,
    plus,
    square,
    full_square,
    diamond,
    horizontal,
    vertical
};

template <>
auto format(PointShape shape) -> std::string;

auto draw_point(Context& ctx, point_t point, PointShape shape, color_t color,
                grid_fine_t size) -> void;

auto draw_points(Context& ctx, std::ranges::input_range auto&& points, PointShape shape,
                 color_t color, grid_fine_t size) -> void {
    for (auto&& point : points) {
        draw_point(ctx, point, shape, color, size);
    }
}

//
// Arrow
//

auto draw_arrow(Context& ctx, point_t point, color_t color, orientation_t orientation,
                grid_fine_t size) -> void;

//
// Line
//

struct LineAttributes {
    color_t color {defaults::color_black};
    int stroke_width {defaults::use_view_config_stroke_width};
    bool p0_endcap {false};
    bool p1_endcap {false};
};

auto draw_orthogonal_line(Context& ctx, BLLine line, LineAttributes attributes) -> void;

auto draw_line(Context& ctx, const ordered_line_t line, LineAttributes attributes)
    -> void;

auto draw_line(Context& ctx, const line_t line, LineAttributes attributes) -> void;

auto draw_line(Context& ctx, const line_fine_t line, LineAttributes attributes) -> void;

//
// Rect
//

struct RectAttributes {
    ShapeDrawType draw_type {ShapeDrawType::fill_and_stroke};
    int stroke_width {defaults::use_view_config_stroke_width};
    color_t fill_color {defaults::color_white};
    color_t stroke_color {defaults::color_black};
};

auto draw_rect(Context& ctx, rect_fine_t rect, RectAttributes attributes) -> void;

struct RoundRectAttributes {
    ShapeDrawType draw_type {ShapeDrawType::fill_and_stroke};
    int stroke_width {defaults::use_view_config_stroke_width};
    grid_fine_t rounding {defaults::maximum_rounding};
    color_t fill_color {defaults::color_white};
    color_t stroke_color {defaults::color_black};
};

auto draw_round_rect(Context& ctx, rect_fine_t rect, RoundRectAttributes attributes)
    -> void;

//
// Circle
//

struct CircleAttributes {
    ShapeDrawType draw_type {ShapeDrawType::fill_and_stroke};
    int stroke_width {defaults::use_view_config_stroke_width};
    color_t fill_color {defaults::color_white};
    color_t stroke_color {defaults::color_black};
};

auto draw_circle(Context& ctx, point_fine_t center, grid_fine_t radius,
                 CircleAttributes attributes) -> void;

//
// Text
//

struct TextAttributes {
    grid_fine_t font_size {1.0};
    color_t color {defaults::color_black};

    HTextAlignment horizontal_alignment {HTextAlignment::left};
    VTextAlignment vertical_alignment {VTextAlignment::baseline};
    FontStyle style {FontStyle::regular};

    // don't render, if scaled font size is smaller
    double cuttoff_size_px {3.0};  // pixels
};

auto draw_text(Context& ctx, point_fine_t position, std::string_view text,
               TextAttributes attributes) -> void;

//
// Icon
//

struct IconAttributes {
    grid_fine_t icon_height {1.0};
    color_t color {defaults::color_black};
    HorizontalAlignment horizontal_alignment {HorizontalAlignment::left};
    VerticalAlignment vertical_alignment {VerticalAlignment::top};
};

auto draw_icon(Context& ctx, point_fine_t position, icon_t icon,
               IconAttributes attributes) -> void;

}  // namespace logicsim

#endif