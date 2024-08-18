#include "render/primitive/line.h"

#include "render/context.h"
#include "vocabulary/line.h"
#include "vocabulary/line_fine.h"
#include "vocabulary/ordered_line.h"

#include <blend2d.h>

#include <cassert>

namespace logicsim {

auto _draw_orthogonal_line_ordered(Context& ctx, const BLLine line,
                                   LineAttributes attributes) -> void {
    assert(line.x0 <= line.x1);
    assert(line.y0 <= line.y1);

    const int stroke_width = resolve_stroke_width(attributes.stroke_width, ctx);

    if (stroke_width < 1) {
        return;
    }

    const int offset = (stroke_width - 1) / 2;

    const int p0_cap = attributes.p0_endcap ? offset : 0;
    const int p1_cap = attributes.p1_endcap ? stroke_width - offset : 0;

    if (line.y0 == line.y1) {
        const auto x = line.x0 - p0_cap;
        const auto w = line.x1 + p1_cap - x;
        ctx.bl_ctx.fillRect(x, line.y0 - offset, w, stroke_width, attributes.color);
    }

    else {
        const auto y = line.y0 - p0_cap;
        const auto h = line.y1 + p1_cap - y;
        ctx.bl_ctx.fillRect(line.x0 - offset, y, stroke_width, h, attributes.color);
    }
}

auto draw_orthogonal_line(Context& ctx, BLLine line, LineAttributes attributes) -> void {
    if (line.x0 > line.x1) {
        std::swap(line.x0, line.x1);
        std::swap(attributes.p0_endcap, attributes.p1_endcap);

    } else if (line.y0 > line.y1) {
        std::swap(line.y0, line.y1);
        std::swap(attributes.p0_endcap, attributes.p1_endcap);
    }

    _draw_orthogonal_line_ordered(ctx, line, attributes);
}

auto draw_line(Context& ctx, const ordered_line_t line,
               LineAttributes attributes) -> void {
    draw_line(ctx, line_fine_t {line}, attributes);
}

auto draw_line(Context& ctx, const line_t line, LineAttributes attributes) -> void {
    draw_line(ctx, line_fine_t {line}, attributes);
}

auto draw_line(Context& ctx, const line_fine_t line, LineAttributes attributes) -> void {
    const auto [x0, y0] = to_context(line.p0, ctx);
    const auto [x1, y1] = to_context(line.p1, ctx);

    draw_orthogonal_line(ctx, BLLine {x0, y0, x1, y1}, attributes);
}

}  // namespace logicsim
