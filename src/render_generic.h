#ifndef LOGIKSIM_RENDER_GENERIC_H
#define LOGIKSIM_RENDER_GENERIC_H

#include "allocated_size.h"
#include "format.h"
#include "glyph_cache.h"
#include "glyph_cache_type.h"
#include "scene.h"
#include "segment_tree_type.h"
#include "vocabulary.h"

#include <functional>

class BLContext;

namespace logicsim {

struct OldRenderSettings;

namespace defaults {
constexpr inline static auto use_view_config_stroke_width = int {-1};
constexpr inline static auto maximum_rounding = grid_fine_t {-1};
}  // namespace defaults

//
// Element Draw State
//

enum class ElementDrawState : uint32_t {
    // inserted
    normal,
    normal_selected,
    valid,
    simulated,

    // uninserted
    colliding,
    temporary_selected,
};

template <>
auto format(ElementDrawState) -> std::string;

[[nodiscard]] auto is_inserted(ElementDrawState state) noexcept -> bool;

[[nodiscard]] auto has_overlay(ElementDrawState state) noexcept -> bool;

//
// Drawable Element
//

struct DrawableElement {
    element_id_t element_id;
    ElementDrawState state;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const DrawableElement& other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const DrawableElement& other) const = default;
};

static_assert(sizeof(DrawableElement) == 8);

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
                     const OldRenderSettings& settings,
                     std::function<void(BLContext&)> render_func) -> void;

//
// Layers Cache
//

struct InteractiveLayers {
    // inserted
    std::vector<DrawableElement> normal_below;
    std::vector<element_id_t> normal_wires;
    std::vector<DrawableElement> normal_above;

    // uninserted
    std::vector<DrawableElement> uninserted_below;
    std::vector<DrawableElement> uninserted_above;

    // selected & temporary
    std::vector<element_id_t> selected_logic_items;
    std::vector<ordered_line_t> selected_wires;
    std::vector<segment_info_t> temporary_wires;
    // valid
    std::vector<element_id_t> valid_logic_items;
    std::vector<ordered_line_t> valid_wires;
    // colliding
    std::vector<element_id_t> colliding_logic_items;
    std::vector<segment_info_t> colliding_wires;

   public:
    std::optional<rect_t> uninserted_bounding_rect;
    std::optional<rect_t> overlay_bounding_rect;

   public:
    auto format() const -> std::string;
    auto clear() -> void;
    auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto has_inserted() const -> bool;
    [[nodiscard]] auto has_uninserted() const -> bool;
    [[nodiscard]] auto has_overlay() const -> bool;

    auto calculate_overlay_bounding_rect() -> void;
};

auto update_uninserted_rect(InteractiveLayers& layers, rect_t bounding_rect) -> void;
auto update_uninserted_rect(InteractiveLayers& layers, ordered_line_t line) -> void;
auto update_overlay_rect(InteractiveLayers& layers, rect_t bounding_rect) -> void;
auto update_overlay_rect(InteractiveLayers& layers, ordered_line_t line) -> void;

//
// Simulation Layers Cache
//

struct SimulationLayers {
    // inserted
    std::vector<element_id_t> items_below;
    std::vector<element_id_t> wires;
    std::vector<element_id_t> items_above;

   public:
    auto format() const -> std::string;
    auto clear() -> void;
    auto allocated_size() const -> std::size_t;
};

// template <>
// struct ByteSize<SimulationLayers> {
//     static auto get_bytesize(const SimulationLayers& cache) -> std::size_t;
// };

//
// OldRenderSettings
//

struct RenderConfig {
    ViewConfig view_config {};

    int background_grid_min_distance {10};  // device pixels
    int thread_count {4};

    auto format() const -> std::string;
};

struct ContextSurfaces {
    LayerSurface layer_surface_uninserted {.enabled = true};
    LayerSurface layer_surface_overlay {.enabled = true};
};

// cache vectors to continuous avoid allocations
struct ContextLayers {
    InteractiveLayers interactive_layers {};
    SimulationLayers simulation_layers {};
};

struct Context {
    BLImage bl_image;
    BLContext bl_ctx;
    RenderConfig settings;

    GlyphCache text_cache {};
    ContextLayers layers;
    ContextSurfaces surfaces;
};

// TODO think about const behavior?
// TODO rename settings to RenderCache ?
// TODO maybe make glyph cache a pointer - to remove dependency
struct OldRenderSettings {
    ViewConfig view_config {};

    // EmptyGlyphCache text {};
    GlyphCache text {};
    mutable InteractiveLayers layers {};
    mutable SimulationLayers simulation_layers {};

    mutable LayerSurface layer_surface_uninserted {.enabled = true};
    mutable LayerSurface layer_surface_overlay {.enabled = true};

    int background_grid_min_distance {10};  // device pixels
    int thread_count {4};

    auto format() const -> std::string;
};

auto context_info(const OldRenderSettings& settings) -> BLContextCreateInfo;

//
// Error checks
//

auto check_errors(BLContext& ctx) -> void;

auto checked_sync(BLContext& ctx) -> void;

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

[[nodiscard]] auto do_fill(DrawType type) -> bool;
[[nodiscard]] auto do_stroke(DrawType type) -> bool;

//
// Strokes
//

auto resolve_stroke_width(int attribute, const OldRenderSettings& settings) -> int;

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

auto draw_point(BLContext& ctx, point_t point, PointShape shape, color_t color,
                double size, const OldRenderSettings& settings) -> void;

auto draw_points(BLContext& ctx, std::ranges::input_range auto&& points, PointShape shape,
                 color_t color, double size, const OldRenderSettings& settings) -> void {
    for (auto&& point : points) {
        draw_point(ctx, point, shape, color, size, settings);
    }
}

//
// Arrow
//

auto draw_arrow(BLContext& ctx, point_t point, color_t color, orientation_t orientation,
                double size, const OldRenderSettings& settings) -> void;

//
// Line
//

struct LineAttributes {
    color_t color {defaults::color_black};
    int stroke_width {defaults::use_view_config_stroke_width};
    bool p0_endcap {false};
    bool p1_endcap {false};
};

auto draw_orthogonal_line(BLContext& ctx, BLLine line, LineAttributes attributes,
                          const OldRenderSettings& settings) -> void;

auto draw_line(BLContext& ctx, const ordered_line_t line, LineAttributes attributes,
               const OldRenderSettings& settings) -> void;

auto draw_line(BLContext& ctx, const line_t line, LineAttributes attributes,
               const OldRenderSettings& settings) -> void;

auto draw_line(BLContext& ctx, const line_fine_t line, LineAttributes attributes,
               const OldRenderSettings& settings) -> void;

//
// Rect
//

struct RectAttributes {
    DrawType draw_type {DrawType::fill_and_stroke};
    int stroke_width {defaults::use_view_config_stroke_width};
    color_t fill_color {defaults::color_white};
    color_t stroke_color {defaults::color_black};
};

auto draw_rect(BLContext& ctx, rect_fine_t rect, RectAttributes attributes,
               const OldRenderSettings& settings) -> void;

struct RoundRectAttributes {
    DrawType draw_type {DrawType::fill_and_stroke};
    int stroke_width {defaults::use_view_config_stroke_width};
    grid_fine_t rounding {defaults::maximum_rounding};
    color_t fill_color {defaults::color_white};
    color_t stroke_color {defaults::color_black};
};

auto draw_round_rect(BLContext& ctx, rect_fine_t rect, RoundRectAttributes attributes,
                     const OldRenderSettings& settings) -> void;

//
// Circle
//

struct CircleAttributes {
    DrawType draw_type {DrawType::fill_and_stroke};
    int stroke_width {defaults::use_view_config_stroke_width};
    color_t fill_color {defaults::color_white};
    color_t stroke_color {defaults::color_black};
};

auto draw_circle(BLContext& ctx, point_fine_t center, grid_fine_t radius,
                 CircleAttributes attributes, const OldRenderSettings& settings) -> void;

//
// Text
//

struct TextAttributes {
    double font_size {1.0};  // grid size
    color_t color {defaults::color_black};

    HorizontalAlignment horizontal_alignment {HorizontalAlignment::left};
    VerticalAlignment vertical_alignment {VerticalAlignment::baseline};
    FontStyle style {FontStyle::regular};

    // don't render, if scaled font size is smaller
    double cuttoff_size_px {3.0};  // pixels
};

auto draw_text(BLContext& ctx, point_fine_t position, std::string_view text,
               TextAttributes attributes, const OldRenderSettings& settings) -> void;

}  // namespace logicsim

#endif