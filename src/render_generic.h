#ifndef LOGIKSIM_RENDER_GENERIC_H
#define LOGIKSIM_RENDER_GENERIC_H

#include "format.h"
#include "glyph_cache.h"
#include "scene.h"
#include "segment_tree_type.h"
#include "vocabulary.h"

#include <functional>

class BLContext;

namespace logicsim {

struct RenderSettings;

namespace defaults {
constexpr inline static auto use_view_config_stroke_width = int {-1};
}

//
// Layer Surface
//
struct LayerSurface {
    bool enabled {true};
    BLImage image {};
    BLContext ctx {};

    auto is_initialized(const ViewConfig& config) const -> bool;
    auto initialize(const ViewConfig& config, const BLContextCreateInfo& info) -> void;
};

auto get_dirty_rect(rect_t bounding_rect, const ViewConfig& view_config) -> BLRectI;

auto render_to_layer(BLContext& target_ctx, LayerSurface& layer, BLRectI dirty_rect,
                     const RenderSettings& settings,
                     std::function<void(BLContext&)> render_func) -> void;

//
// Layers Cache
//

struct LayersCache {
    // inserted
    std::vector<element_id_t> normal_below;
    std::vector<element_id_t> normal_wires;
    std::vector<element_id_t> normal_above;

    // uninserted
    std::vector<element_id_t> uninserted_below;
    std::vector<segment_info_t> uninserted_wires;
    std::vector<element_id_t> uninserted_above;

    // selected & temporary
    std::vector<element_id_t> selected_logic_items;
    std::vector<ordered_line_t> selected_wires;
    std::vector<ordered_line_t> temporary_wires;
    // valid
    std::vector<element_id_t> valid_logic_items;
    std::vector<ordered_line_t> valid_wires;
    // colliding
    std::vector<element_id_t> colliding_logic_items;
    std::vector<ordered_line_t> colliding_wires;

   public:
    std::optional<rect_t> uninserted_bounding_rect;
    std::optional<rect_t> overlay_bounding_rect;

   public:
    auto format() const -> std::string;
    auto clear() -> void;

    [[nodiscard]] auto has_inserted() const -> bool;
    [[nodiscard]] auto has_uninserted() const -> bool;
    [[nodiscard]] auto has_overlay() const -> bool;

    auto calculate_overlay_bounding_rect() -> void;
};

auto update_uninserted_rect(LayersCache& layers, rect_t bounding_rect) -> void;
auto update_uninserted_rect(LayersCache& layers, ordered_line_t line) -> void;
auto update_overlay_rect(LayersCache& layers, rect_t bounding_rect) -> void;
auto update_overlay_rect(LayersCache& layers, ordered_line_t line) -> void;

// TODO think about const behavior?
// TODO rename settings to RenderCache ?
// TODO maybe make glyph cache a pointer - to remove dependency
struct RenderSettings {
    ViewConfig view_config {};

    GlyphCache text {};
    mutable LayersCache layers {};

    mutable LayerSurface layer_surface_uninserted {.enabled = true};
    mutable LayerSurface layer_surface_overlay {.enabled = true};

    int background_grid_min_distance {10};  // device pixels
    int thread_count {4};

    auto format() const -> std::string;
};

auto context_info(const RenderSettings& settings) -> BLContextCreateInfo;

//
// Context Guard
//

class ContextGuard {
   public:
    explicit ContextGuard(BLContext& ctx);
    ~ContextGuard();

   private:
    BLContext& ctx_;
};

//
// Draw Type
//

enum class DrawType {
    fill,
    stroke,
    fill_and_stroke,
};

template <>
auto format(DrawType type) -> std::string;

//
// Strokes
//

auto resolve_stroke_width(int attribute, const RenderSettings& settings) -> int;

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

// TODO !!! fix naming render vs draws
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
// Line
//

struct LineAttributes {
    color_t color {defaults::color_black};
    int stroke_width {defaults::use_view_config_stroke_width};
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

// TODO use color everywhere
struct RectAttributes {
    DrawType draw_type {DrawType::fill_and_stroke};
    int stroke_width {defaults::use_view_config_stroke_width};

    color_t fill_color {defaults::no_color};
    color_t stroke_color {defaults::no_color};
};

auto draw_rect(BLContext& ctx, rect_fine_t rect, RectAttributes attributes,
               const RenderSettings& settings) -> void;

// TODO add colors to rounded rect
struct RoundRectAttributes {
    DrawType draw_type {DrawType::fill_and_stroke};
    int stroke_width {defaults::use_view_config_stroke_width};
    double rounding {-1};
};

auto draw_round_rect(BLContext& ctx, rect_fine_t rect, RoundRectAttributes attributes,
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