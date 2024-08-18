#include "render/primitive/circle.h"

#include "render/context.h"
#include "vocabulary/grid_fine.h"
#include "vocabulary/point_fine.h"

#include <blend2d.h>

#include <exception>
#include <stdexcept>

namespace logicsim {

auto _draw_circle_fill_and_stroke(Context& ctx, point_fine_t center, grid_fine_t radius,
                                  CircleAttributes attributes) -> void {
    const auto&& [x0, y0] =
        to_context(point_fine_t {center.x - radius, center.y - radius}, ctx);
    const auto&& [x1, y1] =
        to_context(point_fine_t {center.x + radius, center.y + radius}, ctx);

    const auto x = (x0 + x1) / 2;
    const auto y = (y0 + y1) / 2;

    const auto rx = (x1 - x0) / 2;
    const auto ry = (y1 - y0) / 2;

    const auto stroke_width = resolve_stroke_width(attributes.stroke_width, ctx);

    ctx.bl_ctx.fillEllipse(BLEllipse {x, y, rx, ry}, attributes.stroke_color);
    ctx.bl_ctx.fillEllipse(BLEllipse {x, y, rx - stroke_width, ry - stroke_width},
                           attributes.fill_color);
}

auto _draw_circle_fill(Context& ctx, point_fine_t center, grid_fine_t radius,
                       CircleAttributes attributes) -> void {
    static_cast<void>(ctx);
    static_cast<void>(center);
    static_cast<void>(radius);
    static_cast<void>(attributes);

    throw std::runtime_error("not implemented");
}

auto _draw_circle_stroke(Context& ctx, point_fine_t center, grid_fine_t radius,
                         CircleAttributes attributes) -> void {
    static_cast<void>(ctx);
    static_cast<void>(center);
    static_cast<void>(radius);
    static_cast<void>(attributes);

    throw std::runtime_error("not implemented");
}

auto draw_circle(Context& ctx, point_fine_t center, grid_fine_t radius,
                 CircleAttributes attributes) -> void {
    switch (attributes.draw_type) {
        using enum ShapeDrawType;

        case fill_and_stroke:
            _draw_circle_fill_and_stroke(ctx, center, radius, attributes);
            return;
        case fill:
            _draw_circle_fill(ctx, center, radius, attributes);
            return;
        case stroke:
            _draw_circle_stroke(ctx, center, radius, attributes);
            return;
    }
    std::terminate();
}

}  // namespace logicsim
