#include "element/decoration/render_decoration_base.h"

#include "layout.h"
#include "render/circuit/alpha_values.h"
#include "render/primitive/point.h"
#include "render/primitive/text.h"
#include "vocabulary/decoration_type.h"
#include "vocabulary/drawable_element.h"
#include "vocabulary/point_fine.h"

namespace logicsim {

namespace {

auto draw_decoration_text_element(Context& ctx, const Layout& layout,
                                  decoration_id_t decoration_id,
                                  ElementDrawState state) -> void {
    const auto position = layout.decorations().position(decoration_id);
    const auto color = with_alpha_runtime(defaults::color_gray, state);
    const auto size = grid_fine_t {0.25};
    draw_point(ctx, position, PointShape::diamond, color, size);

    const auto text_anchor = point_fine_t {position.x + grid_fine_t {0.5}, position.y};
    const auto text_color = with_alpha_runtime(defaults::color_black, state);
    const auto& text_label = layout.decorations().attrs_text_element(decoration_id).text;

    draw_text(ctx, text_anchor, text_label,
              TextAttributes {
                  .font_size = grid_fine_t {1.0},
                  .color = text_color,

                  .horizontal_alignment = HTextAlignment::left,
                  .vertical_alignment = VTextAlignment::center,
                  .style = FontStyle::regular,
              });
}

}  // namespace

auto draw_decoration_base(Context& ctx, const Layout& layout,
                          decoration_id_t decoration_id, ElementDrawState state) -> void {
    switch (layout.decorations().type(decoration_id)) {
        using enum DecorationType;

        case text_element: {
            draw_decoration_text_element(ctx, layout, decoration_id, state);
            return;
        }
    }
    std::terminate();
}

auto draw_decorations_base(Context& ctx, const Layout& layout,
                           std::span<const DrawableDecoration> elements) -> void {
    for (const auto& entry : elements) {
        draw_decoration_base(ctx, layout, entry.decoration_id, entry.state);
    }
}

auto draw_decorations_base(Context& ctx, const Layout& layout,
                           std::span<const decoration_id_t> elements,
                           ElementDrawState state) -> void {
    for (const auto& entry : elements) {
        draw_decoration_base(ctx, layout, entry, state);
    }
}

}  // namespace logicsim
