#include "core/render/primitive/text.h"

#include "core/render/context.h"
#include "core/render/context_cache.h"
#include "core/render/text_cache.h"
#include "core/vocabulary/point_fine.h"

#include <blend2d.h>

namespace logicsim {

auto draw_text(Context& ctx, point_fine_t position, std::string_view text,
               const TextAttributes& attributes) -> void {
    if (text.empty()) {
        return;
    }
    const auto font_size_px = to_context_unrounded(attributes.font_size, ctx);
    if (font_size_px < attributes.cutoff_size_px) {
        return;
    }

    const auto position_px = to_context(position, ctx);
    const auto font_size_px_float = static_cast<float>(font_size_px);

    ctx.cache.text_cache().draw_text(
        ctx.bl_ctx, position_px, text, font_size_px_float,
        TextCache::TextAttributes {
            .color = attributes.color,
            .horizontal_alignment = attributes.horizontal_alignment,
            .vertical_alignment = attributes.vertical_alignment,
            .style = attributes.style,
        });
}

}  // namespace logicsim
