#include "render/primitive/text.h"

#include "render/context.h"
#include "render/context_cache.h"
#include "vocabulary/point_fine.h"

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

    ctx.cache.text_cache().draw_text(ctx.bl_ctx, position_px, text, font_size_px_float,
                                     attributes.color, attributes.horizontal_alignment,
                                     attributes.vertical_alignment, attributes.style);
}

}  // namespace logicsim
