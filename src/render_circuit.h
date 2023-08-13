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

constexpr static inline auto body_color_normal = color_t {255, 255, 128};
constexpr static inline auto body_color_selected = color_t {224, 224, 224};
constexpr static inline auto body_color_uninserted = color_t {192, 192, 192};
constexpr static inline auto body_stroke_color = defaults::color_black;

constexpr static inline auto overlay_selected = color_t {0, 128, 255, 96};
constexpr static inline auto overlay_valid = color_t {0, 192, 0, 96};
constexpr static inline auto overlay_colliding = color_t {255, 0, 0, 96};

}  // namespace defaults

//
// Element Draw State
//

enum class ElementDrawState {
    normal,
    selected,
    uninserted,
    simulated,
};

template <>
auto format(ElementDrawState) -> std::string;

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