#include "render/primitive/icon.h"

#include "render/context.h"
#include "vocabulary/point_fine.h"

namespace logicsim {

auto draw_icon(Context& ctx, point_fine_t position, icon_t icon,
               IconAttributes attributes) -> void {
    const auto position_px = to_context(position, ctx);
    const auto icon_height_px = to_context_unrounded(attributes.icon_height, ctx);

    ctx.cache.svg_cache().draw_icon(
        ctx.bl_ctx, SVGCache::IconAttributes {
                        .icon = icon,
                        .position = position_px,
                        .height = icon_height_px,
                        .color = attributes.color,
                        .horizontal_alignment = attributes.horizontal_alignment,
                        .vertical_alignment = attributes.vertical_alignment,
                    });
}

}  // namespace logicsim
