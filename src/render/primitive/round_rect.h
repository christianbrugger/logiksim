#ifndef LOGICSIM_RENDER_PRIMITIVE_ROUND_RECT_H
#define LOGICSIM_RENDER_PRIMITIVE_ROUND_RECT_H

#include "render/primitive/stroke.h"
#include "vocabulary/color.h"
#include "vocabulary/grid_fine.h"
#include "vocabulary/shape_draw_type.h"

namespace logicsim {

struct rect_fine_t;
struct Context;

namespace defaults {
constexpr inline static auto maximum_rounding = grid_fine_t {-1};
}  // namespace defaults

struct RoundRectAttributes {
    ShapeDrawType draw_type {ShapeDrawType::fill_and_stroke};
    int stroke_width {defaults::use_view_config_stroke_width};
    grid_fine_t rounding {defaults::maximum_rounding};
    color_t fill_color {defaults::color_white};
    color_t stroke_color {defaults::color_black};
};

auto draw_round_rect(Context& ctx, rect_fine_t rect, RoundRectAttributes attributes)
    -> void;

}  // namespace logicsim

#endif
