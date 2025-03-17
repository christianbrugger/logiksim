#include "core/render/circuit/render_rubber_band.h"

#include "core/editable_circuit.h"
#include "core/render/context.h"
#include "core/render/context_guard.h"
#include "core/render/primitive/rect.h"

namespace logicsim {

namespace defaults {
constexpr static inline auto rubberband_border_color = color_t {0, 100, 179};
constexpr static inline auto rubberband_fill_color = color_t {110, 170, 216, 127};
}  // namespace defaults

auto _draw_rubber_band(Context& ctx, rect_fine_t rect) {
    const auto stroke_width = ctx.view_config().stroke_width();
    const auto offset = stroke_width % 2 == 0 ? 0 : 0.5;

    auto [x0, y0] = to_context(rect.p0, ctx);
    auto [x1, y1] = to_context(rect.p1, ctx);

    auto w = x1 - x0;
    auto h = y1 - y0;

    if (w < 1.0 || h < 1.0) {
        return;
    }

    {
        const auto _ = make_context_guard(ctx);

        ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_OVER);
        ctx.bl_ctx.setStrokeWidth(stroke_width);

        ctx.bl_ctx.fillRect(x0, y0, w, h, defaults::rubberband_fill_color);
        ctx.bl_ctx.strokeRect(x0 + offset, y0 + offset, w, h,
                              defaults::rubberband_border_color);
    }
}

auto render_rubber_band(Context& ctx, const EditableCircuit& editable_circuit) -> void {
    if (const auto rect = editable_circuit.last_visible_selection_rect()) {
        _draw_rubber_band(ctx, *rect);
    }
}

}  // namespace logicsim
