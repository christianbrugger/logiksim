#include "render/primitive/round_rect.h"

#include "render/context.h"
#include "vocabulary/rect_fine.h"

#include <blend2d.h>

namespace logicsim {

auto draw_round_rect(Context& ctx, rect_fine_t rect,
                     RoundRectAttributes attributes) -> void {
    const auto&& [x0, y0] = to_context(rect.p0, ctx);
    const auto&& [x1, y1] = to_context(rect.p1, ctx);

    auto w = x1 - x0;
    auto h = y1 - y0;

    if (attributes.draw_type == ShapeDrawType::fill) {
        ++w;
        ++h;
    }

    w = w == 0 ? 1.0 : w;
    h = h == 0 ? 1.0 : h;

    const auto r = attributes.rounding == defaults::maximum_rounding
                       ? std::min(w, h) / 2.0
                       : to_context(attributes.rounding, ctx);

    if (do_fill(attributes.draw_type)) {
        ctx.bl_ctx.fillRoundRect(BLRoundRect {x0, y0, w, h, r}, attributes.fill_color);
    }

    if (do_stroke(attributes.draw_type)) {
        const auto width = resolve_stroke_width(attributes.stroke_width, ctx);
        const auto offset = stroke_offset(width);

        ctx.bl_ctx.setStrokeWidth(width);
        ctx.bl_ctx.strokeRoundRect(BLRoundRect {x0 + offset, y0 + offset, w, h, r},
                                   attributes.stroke_color);
    }
}

}  // namespace logicsim
