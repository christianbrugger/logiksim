#ifndef LOGIKSIM_RENDER_GENERIC_H
#define LOGIKSIM_RENDER_GENERIC_H

#include "format.h"
#include "glyph_cache.h"
#include "scene.h"
#include "vocabulary.h"
#include "segment_tree_types.h"

class BLContext;

namespace logicsim {

struct LayersCache {
    // inserted
    mutable std::vector<element_id_t> normal_below;
    mutable std::vector<element_id_t> normal_wires;
    mutable std::vector<element_id_t> normal_above;

    // uninserted
    mutable std::vector<element_id_t> uninserted_below;
    mutable std::vector<segment_info_t> uninserted_wires;
    mutable std::vector<element_id_t> uninserted_above;

    // selected & temporary
    mutable std::vector<element_id_t> selected_logic_items;
    mutable std::vector<ordered_line_t> selected_wires;
    mutable std::vector<ordered_line_t> temporary_wires;
    // valid
    mutable std::vector<element_id_t> valid_logic_items;
    mutable std::vector<ordered_line_t> valid_wires;
    // colliding
    mutable std::vector<element_id_t> colliding_logic_items;
    mutable std::vector<ordered_line_t> colliding_wires;

    auto format() const -> std::string;
    auto clear() const -> void;
};

// TODO maybe make glyph cache a pointer
struct RenderSettings {
    ViewConfig view_config {};
    GlyphCache text {};
    LayersCache layers {};

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
// Generic Types & Methods
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

auto get_scene_rect_fine(const BLContext& ctx, ViewConfig view_config) -> rect_fine_t;
auto get_scene_rect(const BLContext& ctx, ViewConfig view_config) -> rect_t;

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

// TODO !!! fix naming render vs draw
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

// TODO add colors to rect somehow
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