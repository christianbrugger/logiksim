#ifndef LOGIKSIM_RENDER_GENERIC_H
#define LOGIKSIM_RENDER_GENERIC_H

#include "glyph_cache.h"
#include "scene.h"
#include "vocabulary.h"

class BLContext;

namespace logicsim {

// TODO maybe make glyph cache a pointer
struct RenderSettings {
    ViewConfig view_config {};
    GlyphCache text {};

    int background_grid_min_distance {10};  // device pixels

    auto format() const -> std::string;
};

//
// Blend2d specific
//

class ContextGuard {
   public:
    explicit ContextGuard(BLContext& ctx);
    ~ContextGuard();

   private:
    BLContext& ctx_;
};

//
// Generic Types
//

namespace defaults {
constexpr inline static auto view_config_width = int {-1};
}

enum class DrawType {
    fill,
    stroke,
    fill_and_stroke,
};

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

auto render_point(BLContext& ctx, point_t point, PointShape shape, color_t color,
                  double size, const RenderSettings& settings) -> void;

auto render_points(BLContext& ctx, std::ranges::input_range auto&& points,
                   PointShape shape, color_t color, double size,
                   const RenderSettings& settings) -> void {
    for (auto&& point : points) {
        render_point(ctx, point, shape, color, size, settings);
    }
}

//
// Arrow
//

auto render_arrow(BLContext& ctx, point_t point, color_t color, orientation_t orientation,
                  double size, const RenderSettings& settings) -> void;

//
// Orthogonal Line
//

struct LineAttributes {
    color_t color {defaults::color_black};
    int stroke_width {defaults::view_config_width};
};

auto draw_orthogonal_line(BLContext& ctx, const BLLine& line, LineAttributes attributes,
                          const RenderSettings& settings) -> void;

auto draw_line(BLContext& ctx, const ordered_line_t& line, LineAttributes attributes,
               const RenderSettings& settings) -> void;

auto draw_line(BLContext& ctx, const line_t& line, LineAttributes attributes,
               const RenderSettings& settings) -> void;

auto draw_line(BLContext& ctx, const line_fine_t& line, LineAttributes attributes,
               const RenderSettings& settings) -> void;

//
// Rect
//

struct RectAttributes {
    DrawType draw_type {DrawType::fill_and_stroke};
    int stroke_width {defaults::view_config_width};
};

auto draw_rect(BLContext& ctx, rect_fine_t rect, RectAttributes attributes,
               const RenderSettings& settings) -> void;

//
// Circuit Primitives
//

auto draw_line_cross_point(BLContext& ctx, const point_t point, bool enabled,
                           const RenderSettings& settings) -> void;

auto draw_line_segment(BLContext& ctx, line_t line, bool enabled,
                       const RenderSettings& settings) -> void;

auto draw_line_segment(BLContext& ctx, line_fine_t line, bool enabled,
                       const RenderSettings& settings) -> void;

}  // namespace logicsim

#endif