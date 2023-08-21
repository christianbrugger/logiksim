#ifndef LOGIKSIM_RENDER_CIRCUIT_H
#define LOGIKSIM_RENDER_CIRCUIT_H

#include "render_generic.h"
#include "simulation_view.h"
#include "vocabulary.h"

#include <gsl/gsl>

#include <span>
#include <string_view>

class BLContext;

namespace logicsim {
//
// forward declarations
//
class Layout;
class Selection;
class SimulationView;

namespace layout {
template <bool Const>
class ElementTemplate;

using Element = ElementTemplate<false>;
using ConstElement = ElementTemplate<true>;
}  // namespace layout

namespace simulation_view {
class ConstElement;
}

//
// Defaults
//

namespace defaults {

constexpr static inline auto connector_cutoff_px = 3.0;       // pixels
constexpr static inline auto logic_item_body_overdraw = 0.4;  // grid values

constexpr static inline auto button_body_overdraw = 0.5;  // grid values
constexpr static inline auto button_body_color = defaults::color_gray_90;
constexpr static inline auto led_radius = 0.45;  // grid values

namespace font {
constexpr static inline auto logic_item_text_color = defaults::color_black;
constexpr static inline auto text_cutoff_px = 3.0;  // pixels

constexpr static inline auto logic_item_label_size = 0.9;   // grid values
constexpr static inline auto binary_value_size = 0.7;       // grid values
constexpr static inline auto buffer_label_size = 0.6;       // grid values
constexpr static inline auto connector_label_size = 0.6;    // grid values
constexpr static inline auto connector_label_margin = 0.2;  // grid values

constexpr static inline auto clock_period_size = 0.7;  // grid values
constexpr static inline auto clock_period_color = defaults::color_orange;

constexpr static inline auto display_ascii_controll_color = defaults::color_dark_orange;
constexpr static inline auto display_normal_color = defaults::color_black;
constexpr static inline auto display_size = logic_item_label_size;  // grid values
constexpr static inline auto display_ascii_control_size = 0.7;      // grid values

}  // namespace font

constexpr static inline auto connector_length = 0.4;        // grid points
constexpr static inline auto inverted_circle_radius = 0.2;  // grid points

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
// Logic Items
//

[[nodiscard]] auto draw_logic_item_above(ElementType type) -> bool;

[[nodiscard]] auto get_logic_item_state(layout::ConstElement element,
                                        const Selection* selection) -> ElementDrawState;

[[nodiscard]] auto get_logic_item_fill_color(ElementDrawState state) -> color_t;
[[nodiscard]] auto get_logic_item_stroke_color(ElementDrawState state) -> color_t;

struct LogicItemRectAttributes {
    std::optional<color_t> custom_fill_color {};
    std::optional<color_t> custom_stroke_color {};
};

auto draw_logic_item_rect(BLContext& ctx, rect_fine_t rect, layout::ConstElement element,
                          ElementDrawState state, const RenderSettings& settings,
                          LogicItemRectAttributes attributes = {}) -> void;

struct LogicItemTextAttributes {
    std::optional<double> custom_font_size {};
    std::optional<color_t> custom_text_color {};
    HorizontalAlignment horizontal_alignment {HorizontalAlignment::center};
    VerticalAlignment vertical_alignment {VerticalAlignment::center};
    FontStyle style {FontStyle::regular};
};

auto draw_logic_item_label(BLContext& ctx, point_fine_t point, std::string_view text,
                           layout::ConstElement element, ElementDrawState state,
                           const RenderSettings& settings,
                           LogicItemTextAttributes attributes = {}) -> void;

auto draw_binary_value(BLContext& ctx, point_fine_t point, bool is_enabled,
                       layout::ConstElement element, ElementDrawState state,
                       const RenderSettings& settings) -> void;
auto draw_binary_false(BLContext& ctx, point_fine_t point, layout::ConstElement element,
                       ElementDrawState state, const RenderSettings& settings) -> void;

auto draw_logic_item_base(BLContext& ctx, layout::ConstElement element,
                          ElementDrawState state, const RenderSettings& settings,
                          std::optional<simulation_view::ConstElement> logic_state = {})
    -> void;

auto draw_logic_items_base(BLContext& ctx, const Layout& layout,
                           std::span<const DrawableElement> elements,
                           const RenderSettings& settings) -> void;

auto draw_logic_items_base(BLContext& ctx, const Layout& layout,
                           std::span<const element_id_t> elements,
                           SimulationView simulation_view, const RenderSettings& settings)
    -> void;

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

auto draw_connector(BLContext& ctx, ConnectorAttributes attributes,
                    const RenderSettings& settings) -> void;

auto draw_logic_item_connectors(BLContext& ctx, layout::ConstElement element,
                                ElementDrawState state, const RenderSettings& settings)
    -> void;

auto draw_logic_item_connectors(BLContext& ctx, layout::ConstElement element,
                                ElementDrawState state, const RenderSettings& settings,
                                simulation_view::ConstElement logic_state) -> void;

auto draw_logic_items_connectors(BLContext& ctx, const Layout& layout,
                                 std::span<const DrawableElement> elements,
                                 const RenderSettings& settings) -> void;

auto draw_logic_items_connectors(BLContext& ctx, const Layout& layout,
                                 std::span<const element_id_t> elements,
                                 SimulationView simulation_view,
                                 const RenderSettings& settings) -> void;

template <std::size_t size>
using string_array = std::array<std::string_view, size>;

struct ConnectorLabels {
    gsl::span<const std::string_view> input_labels {};
    gsl::span<const std::string_view> output_labels {};
};

auto draw_connector_label(BLContext& ctx, point_t position, orientation_t orientation,
                          std::string_view label, ElementDrawState state,
                          const RenderSettings& settings) -> void;

auto draw_connector_labels(BLContext& ctx, ConnectorLabels labels,
                           layout::ConstElement element, ElementDrawState state,
                           const RenderSettings& settings) -> void;

//
// Wires
//

auto wire_color(bool is_enabled) -> color_t;
auto wire_color(bool is_enabled, ElementDrawState state) -> color_t;

auto draw_line_cross_point(BLContext& ctx, const point_t point, bool is_enabled,
                           ElementDrawState state, const RenderSettings& settings)
    -> void;

struct SegmentAttributes {
    bool is_enabled {false};
    bool p0_endcap {false};
    bool p1_endcap {false};
};

auto draw_line_segment(BLContext& ctx, ordered_line_t line, SegmentAttributes attributes,
                       ElementDrawState state, const RenderSettings& settings) -> void;

auto draw_line_segment(BLContext& ctx, line_fine_t line, SegmentAttributes attributes,
                       ElementDrawState state, const RenderSettings& settings) -> void;

auto draw_line_segment(BLContext& ctx, segment_info_t info, bool is_enabled,
                       ElementDrawState state, const RenderSettings& settings) -> void;

//
// Overlay
//

enum class shadow_t : uint8_t {
    selected,
    valid,
    colliding,
};

template <>
auto format(shadow_t state) -> std::string;

auto shadow_color(shadow_t shadow_type) -> color_t;

//
// Layout
//

auto render_layout(BLContext& ctx, const Layout& layout, RenderSettings& settings)
    -> void;

auto render_layout(BLContext& ctx, const Layout& layout, const Selection& selection,
                   RenderSettings& settings) -> void;

//
// Simulation
//

// TODO use SimulationResult instead of Simulation

auto render_simulation(BLContext& ctx, const Layout& layout,
                       SimulationView simulation_view, RenderSettings& settings) -> void;

}  // namespace logicsim

#endif