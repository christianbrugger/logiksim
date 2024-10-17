#ifndef LOGICSIM_RENDER_PRIMITIVE_CIRCLE_H
#define LOGICSIM_RENDER_PRIMITIVE_CIRCLE_H

#include "core/render/primitive/stroke.h"
#include "core/vocabulary/color.h"
#include "core/vocabulary/shape_draw_type.h"

namespace logicsim {

struct grid_fine_t;
struct point_fine_t;
struct Context;

struct CircleAttributes {
    ShapeDrawType draw_type {ShapeDrawType::fill_and_stroke};
    int stroke_width {defaults::use_view_config_stroke_width};
    color_t fill_color {defaults::color_white};
    color_t stroke_color {defaults::color_black};
};

auto draw_circle(Context& ctx, point_fine_t center, grid_fine_t radius,
                 CircleAttributes attributes) -> void;

}  // namespace logicsim

#endif
