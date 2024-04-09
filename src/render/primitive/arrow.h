#ifndef LOGICSIM_RENDER_PRIMITIVE_ARROW_H
#define LOGICSIM_RENDER_PRIMITIVE_ARROW_H

#include "vocabulary/orientation.h"

namespace logicsim {

struct point_t;
struct color_t;
struct grid_fine_t;
struct Context;

auto draw_arrow(Context& ctx, point_t point, color_t color, orientation_t orientation,
                grid_fine_t size) -> void;

}  // namespace logicsim

#endif
