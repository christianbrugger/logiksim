#ifndef LOGICSIM_RENDER_PRIMITIVE_LINE_H
#define LOGICSIM_RENDER_PRIMITIVE_LINE_H

#include "render/primitive/stroke.h"
#include "vocabulary/color.h"

struct BLLine;

namespace logicsim {

struct line_t;
struct line_fine_t;
struct ordered_line_t;

struct LineAttributes {
    color_t color {defaults::color_black};
    int stroke_width {defaults::use_view_config_stroke_width};
    bool p0_endcap {false};
    bool p1_endcap {false};
};

auto draw_orthogonal_line(Context& ctx, BLLine line, LineAttributes attributes) -> void;

auto draw_line(Context& ctx, ordered_line_t line, LineAttributes attributes) -> void;

auto draw_line(Context& ctx, line_t line, LineAttributes attributes) -> void;

auto draw_line(Context& ctx, line_fine_t line, LineAttributes attributes) -> void;

}  // namespace logicsim

#endif
