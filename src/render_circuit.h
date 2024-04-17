#ifndef LOGIKSIM_RENDER_CIRCUIT_H
#define LOGIKSIM_RENDER_CIRCUIT_H

#include "exception.h"
#include "logic_item/layout_display.h"  // TODO remove
#include "render_generic.h"
#include "simulation_view.h"
#include "vocabulary.h"
#include "vocabulary/drawable_element.h"
#include "vocabulary/element_draw_state.h"
#include "vocabulary/logicitem_id.h"
#include "vocabulary/logicitem_type.h"
#include "vocabulary/segment_info.h"
#include "vocabulary/wire_id.h"

#include <gsl/gsl>

#include <span>
#include <string_view>

namespace logicsim {
// forward declarations

struct logicitem_id_t;
class Layout;
class Selection;
class SimulationView;
struct size_handle_t;
struct segment_info_t;

namespace simulation_view {
class ConstElement;
}  // namespace simulation_view

//
// Defaults
//

namespace defaults {

constexpr static inline auto connector_cutoff_px = 3.0;  // pixels

constexpr static inline auto button_body_color = defaults::color_gray_90;
constexpr static inline auto led_radius = grid_fine_t {0.45};

namespace font {
constexpr static inline auto logic_item_text_color = defaults::color_black;
constexpr static inline auto text_cutoff_px = 3.0;  // pixels

constexpr static inline auto logic_item_label_size = grid_fine_t {0.9};
constexpr static inline auto binary_value_size = grid_fine_t {0.7};
constexpr static inline auto buffer_label_size = grid_fine_t {0.6};
constexpr static inline auto connector_label_size = grid_fine_t {0.6};
constexpr static inline auto connector_label_margin = grid_fine_t {0.2};

constexpr static inline auto clock_name_size = grid_fine_t {0.7};
constexpr static inline auto clock_name_color = defaults::color_black;
constexpr static inline auto clock_name_style = FontStyle::bold;
constexpr static inline auto clock_period_size = grid_fine_t {0.7};
constexpr static inline auto clock_period_color = defaults::color_black;
constexpr static inline auto clock_period_style = FontStyle::regular;

constexpr static inline auto display_ascii_controll_color = defaults::color_dark_orange;
constexpr static inline auto display_normal_color = defaults::color_black;
constexpr static inline auto display_font_style = display::font_style;
constexpr static inline auto display_font_size = display::font_size;
constexpr static inline auto display_ascii_control_size = grid_fine_t {0.7};

}  // namespace font

constexpr static inline auto connector_length = grid_fine_t {0.4};
constexpr static inline auto inverted_circle_radius = grid_fine_t {0.2};
constexpr static inline auto size_handle_color_fill = defaults::color_orange;
constexpr static inline auto size_handle_color_stroke = defaults::color_dark_orange;
constexpr static inline auto setting_handle_color_fill = defaults::color_orange;
constexpr static inline auto setting_handle_color_stroke = defaults::color_dark_orange;
constexpr static inline auto setting_handle_color_icon = defaults::color_white;
constexpr static inline auto setting_handle_icon_scale = 0.7;  // ratio vs button size

namespace element_state_alpha {
constexpr static inline auto normal = color_t::value_type {255};
constexpr static inline auto colliding = color_t::value_type {64};
constexpr static inline auto temporary = color_t::value_type {128};
}  // namespace element_state_alpha

namespace body_fill_color {
constexpr static inline auto normal = color_t {255, 255, 128};
constexpr static inline auto normal_selected = color_t {224, 224, 224};
constexpr static inline auto valid = color_t {192, 192, 192};
constexpr static inline auto colliding = color_t {192, 192, 192};
constexpr static inline auto temporary_selected = color_t {192, 192, 192};
}  // namespace body_fill_color

constexpr static inline auto body_stroke_color = defaults::color_black;
constexpr static inline auto inverted_connector_fill = defaults::color_white;
constexpr static inline auto wire_color_disabled = defaults::color_black;
constexpr static inline auto wire_color_enabled = defaults::color_red;
constexpr static inline auto led_color_disabled = defaults::color_light_gray;
constexpr static inline auto led_color_enabled = defaults::color_red;

namespace overlay_color {
constexpr static inline auto selected = color_t {0, 128, 255, 96};
constexpr static inline auto valid = color_t {0, 192, 0, 96};
constexpr static inline auto colliding = color_t {255, 0, 0, 96};
}  // namespace overlay_color

}  // namespace defaults

//
// Colors
//

constexpr auto state_alpha(ElementDrawState state) noexcept -> color_t::value_type {
    switch (state) {
        using enum ElementDrawState;

        case normal:
        case normal_selected:
        case valid:
        case simulated:
            return defaults::element_state_alpha::normal;

        case colliding:
            return defaults::element_state_alpha::colliding;
        case temporary_selected:
            return defaults::element_state_alpha::temporary;
    }
    throw_exception("unexcepted draw state in state_alpha");
}

consteval auto with_alpha(color_t color, ElementDrawState state) noexcept -> color_t {
    return color_t {color.r(), color.g(), color.b(), state_alpha(state)};
}

constexpr auto with_alpha_runtime(color_t color, ElementDrawState state) noexcept
    -> color_t {
    return color_t {color.r(), color.g(), color.b(), state_alpha(state)};
}

//
// Background
//

auto render_background(Context& ctx) -> void;

//
// Connectors
//

struct ConnectorAttributes {
    ElementDrawState state;
    point_t position;
    orientation_t orientation;
    bool is_inverted;
    bool is_enabled;
};

auto draw_connector(Context& ctx, ConnectorAttributes attributes) -> void;

auto draw_logic_item_connectors(Context& ctx, const Layout& layout,
                                logicitem_id_t logicitem_id, ElementDrawState state)
    -> void;

auto draw_logic_item_connectors(Context& ctx, const Layout& layout,
                                logicitem_id_t logicitem_id, ElementDrawState state,
                                simulation_view::ConstElement logic_state) -> void;

auto draw_logic_items_connectors(Context& ctx, const Layout& layout,
                                 std::span<const DrawableElement> elements) -> void;

auto draw_logic_items_connectors(Context& ctx, const Layout& layout,
                                 std::span<const logicitem_id_t> elements,
                                 SimulationView simulation_view) -> void;

template <std::size_t size>
using string_array = std::array<std::string_view, size>;

struct ConnectorLabels {
    gsl::span<const std::string_view> input_labels {};
    gsl::span<const std::string_view> output_labels {};
};

auto draw_connector_label(Context& ctx, point_t position, orientation_t orientation,
                          std::string_view label, ElementDrawState state) -> void;

auto draw_connector_labels(Context& ctx, const Layout& layout,
                           logicitem_id_t logicitem_id, ConnectorLabels labels,
                           ElementDrawState state) -> void;

//
// Logic Items
//

[[nodiscard]] auto draw_logic_item_above(LogicItemType type) -> bool;

[[nodiscard]] auto get_logic_item_state(const Layout& layout, logicitem_id_t logicitem_id,
                                        const Selection* selection) -> ElementDrawState;

[[nodiscard]] auto get_logic_item_fill_color(ElementDrawState state) -> color_t;
[[nodiscard]] auto get_logic_item_stroke_color(ElementDrawState state) -> color_t;
[[nodiscard]] auto get_logic_item_text_color(ElementDrawState state) -> color_t;

[[nodiscard]] auto get_logic_item_center(const Layout& layout,
                                         logicitem_id_t logicitem_id) -> point_fine_t;

struct LogicItemRectAttributes {
    std::optional<color_t> custom_fill_color {};
    std::optional<color_t> custom_stroke_color {};
};

auto draw_logic_item_rect(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                          ElementDrawState state, LogicItemRectAttributes attributes = {})
    -> void;

auto draw_logic_item_rect(Context& ctx, rect_fine_t rect, ElementDrawState state,
                          LogicItemRectAttributes attributes = {}) -> void;

struct LogicItemTextAttributes {
    std::optional<grid_fine_t> custom_font_size {};
    std::optional<color_t> custom_text_color {};
    HTextAlignment horizontal_alignment {HTextAlignment::center};
    VTextAlignment vertical_alignment {VTextAlignment::center};
    FontStyle style {FontStyle::regular};
};

auto draw_logic_item_label(Context& ctx, const Layout& layout,
                           logicitem_id_t logicitem_id, std::string_view text,
                           ElementDrawState state,
                           LogicItemTextAttributes attributes = {}) -> void;

auto draw_logic_item_label(Context& ctx, point_fine_t center, std::string_view text,
                           ElementDrawState state,
                           LogicItemTextAttributes attributes = {}) -> void;

auto draw_binary_value(Context& ctx, point_fine_t point, bool is_enabled,
                       ElementDrawState state) -> void;
auto draw_binary_false(Context& ctx, point_fine_t point, ElementDrawState state) -> void;

auto draw_logic_item_base(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                          ElementDrawState state,
                          std::optional<simulation_view::ConstElement> logic_state = {})
    -> void;

auto draw_logic_items_base(Context& ctx, const Layout& layout,
                           std::span<const DrawableElement> elements) -> void;

auto draw_logic_items_base(Context& ctx, const Layout& layout,
                           std::span<const logicitem_id_t> elements,
                           SimulationView simulation_view) -> void;

//
// Wires
//

auto wire_color(bool is_enabled) -> color_t;
auto wire_color(bool is_enabled, ElementDrawState state) -> color_t;

auto draw_line_cross_point(Context& ctx, point_t point, bool is_enabled,
                           ElementDrawState state) -> void;

struct SegmentAttributes {
    bool is_enabled {false};
    bool p0_endcap {false};
    bool p1_endcap {false};
};

auto draw_line_segment(Context& ctx, ordered_line_t line, SegmentAttributes attributes,
                       ElementDrawState state) -> void;

auto draw_line_segment(Context& ctx, line_fine_t line, SegmentAttributes attributes,
                       ElementDrawState state) -> void;

auto draw_line_segment(Context& ctx, segment_info_t info, bool is_enabled,
                       ElementDrawState state) -> void;

//
// Handles
//

auto render_size_handles(Context& ctx, const Layout& layout, const Selection& selection)
    -> void;

auto render_setting_handle(Context& ctx, const Layout& layout, const Selection& selection)
    -> void;

//
// Overlay
//

enum class shadow_t : uint8_t {
    selected,
    valid,
    colliding,
};

template <>
[[nodiscard]] auto format(shadow_t state) -> std::string;

auto shadow_color(shadow_t shadow_type) -> color_t;

//
// Interactive Layers
//

struct InteractiveLayers {
    // inserted
    std::vector<DrawableElement> normal_below;
    std::vector<wire_id_t> normal_wires;
    std::vector<DrawableElement> normal_above;

    // uninserted
    std::vector<DrawableElement> uninserted_below;
    std::vector<DrawableElement> uninserted_above;

    // selected & temporary
    std::vector<logicitem_id_t> selected_logic_items;
    std::vector<ordered_line_t> selected_wires;
    std::vector<segment_info_t> temporary_wires;
    // valid
    std::vector<logicitem_id_t> valid_logic_items;
    std::vector<ordered_line_t> valid_wires;
    // colliding
    std::vector<logicitem_id_t> colliding_logic_items;
    std::vector<segment_info_t> colliding_wires;

   public:
    std::optional<rect_t> uninserted_bounding_rect;
    std::optional<rect_t> overlay_bounding_rect;

   public:
    [[nodiscard]] auto format() const -> std::string;
    auto clear() -> void;
    auto shrink_to_fit() -> void;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

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
// Simulation Layers
//

struct SimulationLayers {
    // inserted
    std::vector<logicitem_id_t> items_below;
    std::vector<wire_id_t> wires;
    std::vector<logicitem_id_t> items_above;

   public:
    [[nodiscard]] auto format() const -> std::string;
    auto clear() -> void;
    auto shrink_to_fit() -> void;
    [[nodiscard]] auto allocated_size() const -> std::size_t;
};

//
// Circuit Context
//

struct CircuitSurfaces {
    LayerSurface layer_surface_uninserted {.enabled = true};
    LayerSurface layer_surface_overlay {.enabled = true};

    auto clear() -> void;
    auto shrink_to_fit() -> void;
};

struct CircuitLayers {
    // vectors are cached to avoid allocations
    InteractiveLayers interactive_layers {};
    SimulationLayers simulation_layers {};

    auto clear() -> void;
    auto shrink_to_fit() -> void;
    [[nodiscard]] auto allocated_size() const -> std::size_t;
};

struct CircuitContext {
    Context ctx {};

    CircuitLayers layers {};
    CircuitSurfaces surfaces {};

    auto clear() -> void;
    auto shrink_to_fit() -> void;
};

//
// Layout
//

auto render_layout(CircuitContext& circuit_ctx, const Layout& layout) -> void;

auto render_layout(CircuitContext& circuit_ctx, const Layout& layout,
                   const Selection& selection) -> void;

auto render_layout_to_file(const Layout& layout, int width, int height,
                           const std::filesystem::path& filename,
                           const ViewConfig& view_config = {}) -> void;
auto render_layout_to_file(const Layout& layout, const Selection& selection, int width,
                           int height, const std::filesystem::path& filename,
                           const ViewConfig& view_config = {}) -> void;

//
// Simulation
//

auto render_simulation(CircuitContext& circuit_ctx, const Layout& layout,
                       SimulationView simulation_view) -> void;

auto render_simulation_to_file(const Layout& layout, SimulationView simulation_view,
                               int width, int height,
                               const std::filesystem::path& filename,
                               const ViewConfig& view_config = {}) -> void;

}  // namespace logicsim

#endif
