#ifndef LOGICSIM_RENDER_PRIMITIVE_POINT_H
#define LOGICSIM_RENDER_PRIMITIVE_POINT_H

#include "concept/input_range.h"
#include "format/enum.h"
#include "vocabulary/color.h"
#include "vocabulary/grid_fine.h"

#include <string>

namespace logicsim {

struct point_t;
struct Context;

enum class PointShape {
    circle,
    full_circle,
    cross,
    plus,
    square,
    full_square,
    diamond,
    horizontal,
    vertical
};

template <>
auto format(PointShape shape) -> std::string;

auto draw_point(Context& ctx, point_t point, PointShape shape, color_t color,
                grid_fine_t size) -> void;

auto draw_points(Context& ctx, input_range_of<point_t> auto&& points, PointShape shape,
                 color_t color, grid_fine_t size) -> void {
    for (auto&& point : points) {
        draw_point(ctx, point, shape, color, size);
    }
}

}  // namespace logicsim

#endif
