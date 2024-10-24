#include "core/render/primitive/text.h"

#include "core/render/bl_box.h"
#include "core/render/context.h"
#include "core/render/context_cache.h"
#include "core/render/text_cache.h"
#include "core/vocabulary/point_fine.h"

#include <blend2d.h>

namespace logicsim {

auto draw_text(Context& ctx, point_fine_t position, std::string_view text,
               const TextAttributes& attributes) -> draw_text_result_t {
    if (text.empty()) {
        return draw_text_result_t {
            .truncated = TextTruncated::no,
            .bounding_box = empty_bl_box,
        };
    }
    const auto font_size_px = to_context_unrounded(attributes.font_size, ctx);
    if (font_size_px < attributes.cutoff_size_px) {
        return draw_text_result_t {
            .truncated = TextTruncated::no,
            .bounding_box = empty_bl_box,
        };
    }

    const auto position_px = to_context(position, ctx);
    const auto font_size_px_float = static_cast<float>(font_size_px);
    const auto max_text_width_px =
        attributes.max_text_width
            ? std::make_optional(to_context_unrounded(*attributes.max_text_width, ctx))
            : std::nullopt;

    return ctx.cache.text_cache().draw_text(
        ctx.bl_ctx, position_px, text, font_size_px_float,
        TextCache::TextAttributes {
            .color = attributes.color,
            .horizontal_alignment = attributes.horizontal_alignment,
            .vertical_alignment = attributes.vertical_alignment,
            .style = attributes.style,
            .max_text_width = max_text_width_px,
        });
}

}  // namespace logicsim
