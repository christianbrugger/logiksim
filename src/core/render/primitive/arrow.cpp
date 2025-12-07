#include "core/render/primitive/arrow.h"

#include "core/geometry/orientation.h"
#include "core/render/context.h"
#include "core/render/context_guard.h"
#include "core/vocabulary/color.h"
#include "core/vocabulary/grid_fine.h"
#include "core/vocabulary/point.h"

namespace logicsim {

auto draw_arrow(Context& ctx, point_t point, color_t color, orientation_t orientation,
                grid_fine_t size) -> void {
    auto _ [[maybe_unused]] = make_context_guard(ctx);

    ctx.bl_ctx.set_stroke_width(1);
    ctx.bl_ctx.set_stroke_style(color);

    const auto [x, y] = to_context(point, ctx);
    const auto d = to_context(size, ctx);
    const auto angle = to_angle(orientation);

    ctx.bl_ctx.translate(BLPoint {x, y});
    ctx.bl_ctx.rotate(angle);

    ctx.bl_ctx.stroke_line(BLLine(0, 0, d, 0));
    ctx.bl_ctx.stroke_line(BLLine(0, 0, d * 0.5, +d * 0.25));
    ctx.bl_ctx.stroke_line(BLLine(0, 0, d * 0.5, -d * 0.25));
}

}  // namespace logicsim
