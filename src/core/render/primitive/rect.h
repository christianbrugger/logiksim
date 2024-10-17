#ifndef LOGICSIM_RENDER_PRIMITIVE_RECT_H
#define LOGICSIM_RENDER_PRIMITIVE_RECT_H

#include "core/render/primitive/stroke.h"
#include "core/vocabulary/color.h"
#include "core/vocabulary/shape_draw_type.h"

namespace logicsim {

struct rect_fine_t;
struct Context;

struct RectAttributes {
    ShapeDrawType draw_type {ShapeDrawType::fill_and_stroke};
    int stroke_width {defaults::use_view_config_stroke_width};
    color_t fill_color {defaults::color_white};
    color_t stroke_color {defaults::color_black};
};

auto draw_rect(Context& ctx, rect_fine_t rect, RectAttributes attributes) -> void;

}  // namespace logicsim

#endif
