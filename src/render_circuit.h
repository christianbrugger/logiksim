#ifndef LOGIKSIM_RENDER_CIRCUIT_H
#define LOGIKSIM_RENDER_CIRCUIT_H

#include "renderer.h"  // TODO !!! remove, when done porting
#include "vocabulary.h"

namespace logicsim {
//
// forward declarations
//
namespace layout {
template <bool Const>
class ElementTemplate;

using Element = ElementTemplate<false>;
using ConstElement = ElementTemplate<true>;
}  // namespace layout

//
// Defaults
//

namespace defaults {

constexpr static inline auto logic_item_body_overdraw = 0.4;  // grid values
constexpr static inline auto button_body_overdraw = 0.5;      // grid values

constexpr static inline auto logic_item_label_size = 0.9;       // grid values
constexpr static inline auto logic_item_label_cutoff_px = 3.0;  // pixels

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

auto draw_logic_item_rect(BLContext& ctx, rect_fine_t rect, layout::ConstElement element,
                          const ElementDrawState state, const RenderSettings& settings)
    -> void;

auto draw_logic_item_text(BLContext& ctx, point_fine_t point, std::string label,
                          layout::ConstElement element, const ElementDrawState state,
                          const RenderSettings& settings);

struct ConnectorAttributes {
    ElementDrawState state;
    point_t position;
    orientation_t orientation;
    bool is_inverted;
    bool is_enabled;
};

auto draw_connector(BLContext& ctx, ConnectorAttributes attributes,
                    const RenderSettings& settings) -> void;

//
//
//

auto draw_logic_items(BLContext& ctx, const Layout& layout,
                      std::span<const DrawableElement> elements,
                      const RenderSettings& settings) -> void;

//
// Wires
//

auto wire_color(bool is_enabled, ElementDrawState state) -> color_t;

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
// Layout Rendering
//

// TODO fix render args
// TODO fix nameing
auto render_circuit_2(BLContext& ctx, render_args_t args) -> void;

}  // namespace logicsim

#endif