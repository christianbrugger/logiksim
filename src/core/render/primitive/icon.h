#ifndef LOGICSIM_RENDER_PRIMITIVE_ICON_H
#define LOGICSIM_RENDER_PRIMITIVE_ICON_H

#include "core/resource.h"
#include "core/vocabulary/alignment.h"
#include "core/vocabulary/color.h"
#include "core/vocabulary/grid_fine.h"

namespace logicsim {

struct point_fine_t;
struct Context;

struct IconAttributes {
    grid_fine_t icon_height {1.0};
    color_t color {defaults::color_black};
    HorizontalAlignment horizontal_alignment {HorizontalAlignment::left};
    VerticalAlignment vertical_alignment {VerticalAlignment::top};
};

auto draw_icon(Context& ctx, point_fine_t position, icon_t icon,
               IconAttributes attributes) -> void;

}  // namespace logicsim

#endif
