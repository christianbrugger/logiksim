#include "core/render/circuit/render_setting_handle.h"

#include "core/geometry/rect.h"
#include "core/render/context.h"
#include "core/render/primitive/icon.h"
#include "core/render/primitive/rect.h"
#include "core/setting_handle.h"
#include "core/vocabulary/color.h"
#include "core/vocabulary/rect_fine.h"

namespace logicsim {

namespace defaults {
constexpr static inline auto setting_handle_color_fill = defaults::color_orange;
constexpr static inline auto setting_handle_color_stroke = defaults::color_dark_orange;
constexpr static inline auto setting_handle_color_icon = defaults::color_white;
constexpr static inline auto setting_handle_icon_scale = 0.7;  // ratio vs button size
}  // namespace defaults

auto draw_setting_handle(Context& ctx, setting_handle_t handle) -> void {
    const auto rect = setting_handle_rect(handle);
    const auto icon_height =
        defaults::setting_handle_size * defaults::setting_handle_icon_scale;

    // button rect
    draw_rect(ctx, rect,
              RectAttributes {
                  .draw_type = ShapeDrawType::fill_and_stroke,
                  .fill_color = defaults::setting_handle_color_fill,
                  .stroke_color = defaults::setting_handle_color_stroke,
              });

    // button icon
    draw_icon(ctx, get_center(rect), handle.icon,
              IconAttributes {
                  .icon_height = icon_height,
                  .color = defaults::setting_handle_color_icon,
                  .horizontal_alignment = HorizontalAlignment::center,
                  .vertical_alignment = VerticalAlignment::center,
              });
}

auto render_setting_handle(Context& ctx, const Layout& layout, const Selection& selection)
    -> void {
    ctx.bl_ctx.set_comp_op(BL_COMP_OP_SRC_COPY);

    if (const auto handle = setting_handle_position(layout, selection)) {
        draw_setting_handle(ctx, *handle);
    }
}

}  // namespace logicsim
