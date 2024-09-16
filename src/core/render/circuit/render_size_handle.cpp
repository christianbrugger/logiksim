#include "render/circuit/render_size_handle.h"

#include "algorithm/round.h"
#include "render/context.h"
#include "size_handle.h"
#include "vocabulary/color.h"

namespace logicsim {

namespace defaults {
constexpr static inline auto size_handle_color_fill = defaults::color_orange;
constexpr static inline auto size_handle_color_stroke = defaults::color_dark_orange;
}  // namespace defaults

namespace {

struct OutlinedRectAttributes {
    color_t fill_color;
    color_t stroke_color;
    double stroke_width_device;
};

auto draw_outlined_rect_px(Context& ctx, BLRect rect, OutlinedRectAttributes attributes) {
    auto stroke_width = std::max(1., round_fast(attributes.stroke_width_device *
                                                ctx.view_config().device_pixel_ratio()));

    // draw square
    ctx.bl_ctx.fillRect(rect, attributes.stroke_color);
    rect.x += stroke_width;
    rect.y += stroke_width;
    rect.w -= stroke_width * 2;
    rect.h -= stroke_width * 2;
    ctx.bl_ctx.fillRect(rect, attributes.fill_color);
}
}  // namespace

auto draw_size_handle(Context& ctx, const size_handle_t& position) -> void {
    auto rect = size_handle_rect_px(position, ctx.view_config());

    draw_outlined_rect_px(
        ctx, rect,
        OutlinedRectAttributes {
            .fill_color = defaults::size_handle_color_fill,
            .stroke_color = defaults::size_handle_color_stroke,
            .stroke_width_device = defaults::size_handle_stroke_width_device,
        });
}

auto draw_size_handles(Context& ctx,
                       std::span<const size_handle_t> handle_positions) -> void {
    for (const auto& position : handle_positions) {
        draw_size_handle(ctx, position);
    }
}

auto render_size_handles(Context& ctx, const Layout& layout,
                         const Selection& selection) -> void {
    ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    draw_size_handles(ctx, size_handle_positions(layout, selection));
}

}  // namespace logicsim
