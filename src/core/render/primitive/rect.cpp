#include "core/render/primitive/rect.h"

#include "core/render/context.h"
#include "core/vocabulary/rect_fine.h"

#include <blend2d/blend2d.h>

#include <exception>

namespace logicsim {

auto _draw_rect_stroke(Context& ctx, rect_fine_t rect, RectAttributes attributes)
    -> void {
    const auto [x0, y0] = to_context(rect.p0, ctx);
    const auto [x1, y1] = to_context(rect.p1, ctx);

    const auto w = std::max(1., x1 - x0);
    const auto h = std::max(1., y1 - y0);

    const auto width = resolve_stroke_width(attributes.stroke_width, ctx);

    ctx.bl_ctx.set_stroke_width(width);
    ctx.bl_ctx.stroke_rect(x0 + width / 2.0, y0 + width / 2.0, w - width, h - width,
                           attributes.stroke_color);
}

auto _draw_rect_fill(Context& ctx, rect_fine_t rect, RectAttributes attributes) -> void {
    const auto [x0, y0] = to_context(rect.p0, ctx);
    const auto [x1, y1] = to_context(rect.p1, ctx);

    const auto w = std::max(1., x1 - x0);
    const auto h = std::max(1., y1 - y0);

    ctx.bl_ctx.fill_rect(x0, y0, w, h, attributes.fill_color);
}

auto _draw_rect_fill_and_stroke(Context& ctx, rect_fine_t rect,
                                RectAttributes attributes) {
    const auto stroke_width = resolve_stroke_width(attributes.stroke_width, ctx);

    auto [x0, y0] = to_context(rect.p0, ctx);
    auto [x1, y1] = to_context(rect.p1, ctx);

    auto w = std::max(1., x1 - x0);
    auto h = std::max(1., y1 - y0);

    if (stroke_width > 0) {
        ctx.bl_ctx.fill_rect(x0, y0, w, h, attributes.stroke_color);

        x0 += stroke_width;
        y0 += stroke_width;
        w -= stroke_width * 2;
        h -= stroke_width * 2;
    }

    if (w >= 1 && h >= 1) {
        ctx.bl_ctx.fill_rect(x0, y0, w, h, attributes.fill_color);
    }
}

auto draw_rect(Context& ctx, rect_fine_t rect, RectAttributes attributes) -> void {
    switch (attributes.draw_type) {
        using enum ShapeDrawType;

        case fill:
            _draw_rect_fill(ctx, rect, attributes);
            return;
        case stroke:
            _draw_rect_stroke(ctx, rect, attributes);
            return;
        case fill_and_stroke:
            _draw_rect_fill_and_stroke(ctx, rect, attributes);
            return;
    }
    std::terminate();
}

}  // namespace logicsim
