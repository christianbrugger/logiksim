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

namespace body_fill_color {
constexpr static inline auto normal = color_t {255, 255, 128, 255};
constexpr static inline auto normal_selected = color_t {224, 224, 224, 255};
constexpr static inline auto valid = color_t {192, 192, 192, 255};
constexpr static inline auto colliding = color_t {192, 192, 192, 64};
constexpr static inline auto temporary_selected = color_t {192, 192, 192, 128};
}  // namespace body_fill_color

namespace body_stroke_color {
constexpr static inline auto normal = color_t {0, 0, 0, 255};
constexpr static inline auto colliding = color_t {0, 0, 0, 64};
constexpr static inline auto temporary_selected = color_t {0, 0, 0, 128};
}  // namespace body_stroke_color

constexpr static inline auto overlay_selected = color_t {0, 128, 255, 96};
constexpr static inline auto overlay_valid = color_t {0, 192, 0, 96};
constexpr static inline auto overlay_colliding = color_t {255, 0, 0, 96};

}  // namespace defaults

//
// Logic Items Generics
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

auto draw_logic_items(BLContext& ctx, const Layout& layout,
                      std::span<const DrawableElement> elements,
                      const RenderSettings& settings) -> void;

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