#include "core/element/decoration/render_decoration_base.h"

#include "core/geometry/offset.h"
#include "core/layout.h"
#include "core/render/circuit/alpha_values.h"
#include "core/render/context.h"
#include "core/render/primitive/point.h"
#include "core/render/primitive/text.h"
#include "core/vocabulary/decoration_type.h"
#include "core/vocabulary/drawable_element.h"
#include "core/vocabulary/point_fine.h"

namespace logicsim {

namespace defaults {

constexpr static inline auto text_element_angle_color = defaults::color_light_gray;
constexpr static inline auto text_element_angle_size = grid_fine_t {0.25};  // (0 - 0.5]

}  // namespace defaults

namespace {

/**
 * @brief: Draw an angle with the tip at origin.
 *
 * Note, shift determines the size and direction. Left for positive, right for negative.
 */
auto draw_decoration_text_angle(Context& ctx, point_fine_t origin, double shift,
                                color_t color) -> void {
    const auto stroke_width = ctx.view_config().stroke_width();
    const auto [x, y] = to_context(origin, ctx);

    const auto poly = std::array {
        BLPoint {x + shift, y - shift},
        BLPoint {x, y},
        BLPoint {x + shift, y + shift},
    };
    const auto view = BLArrayView<BLPoint> {poly.data(), poly.size()};

    ctx.bl_ctx.setStrokeWidth(stroke_width);
    ctx.bl_ctx.strokePolyline(BLArrayView<BLPoint>(view), color);
}

/**
 * @brief: Offset of the angles origin of the text element from the position.
 */
consteval auto text_element_angle_offset() -> point_fine_t {
    static_assert(defaults::text_element_angle_size > grid_fine_t {0});
    static_assert(defaults::text_element_angle_size <= grid_fine_t {0.5});
    return point_fine_t {grid_fine_t {0.25} + defaults::text_element_angle_size / 2, 0};
}

/**
 * @brief: Draw all angles of the text element.
 */
auto draw_decoration_text_angles(Context& ctx, point_t position, size_2d_t size,
                                 ElementDrawState state) -> void {
    const auto angle_offset = text_element_angle_offset();
    const auto color = with_alpha_runtime(defaults::text_element_angle_color, state);
    const auto shift = to_context(defaults::text_element_angle_size, ctx);

    // start angle
    {
        const auto origin_start = position - angle_offset;
        draw_decoration_text_angle(ctx, origin_start, shift, color);
    }

    // end angle
    {
        const auto position_end = point_t {to_grid(size.width, position.x), position.y};
        const auto origin_end = position_end + angle_offset;
        draw_decoration_text_angle(ctx, origin_end, -shift, color);
    }
}

/**
 * @brief: Draw the full text element.
 */
auto draw_decoration_text_element(Context& ctx, const Layout& layout,
                                  decoration_id_t decoration_id,
                                  ElementDrawState state) -> void {
    const auto position = layout.decorations().position(decoration_id);
    const auto size = layout.decorations().size(decoration_id);

    // angles
    draw_decoration_text_angles(ctx, position, size, state);

    // text
    const auto text_anchor = point_fine_t {position.x, position.y};
    const auto text_color = with_alpha_runtime(defaults::color_black, state);
    const auto& text_label = layout.decorations().attrs_text_element(decoration_id).text;
    draw_text(ctx, text_anchor, text_label,
              TextAttributes {
                  .font_size = grid_fine_t {0.9},
                  .color = text_color,

                  .horizontal_alignment = HTextAlignment::left,
                  .vertical_alignment = VTextAlignment::center_baseline,
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
