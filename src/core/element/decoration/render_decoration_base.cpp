#include "core/element/decoration/render_decoration_base.h"

#include "core/algorithm/range_extended.h"
#include "core/geometry/offset.h"
#include "core/layout.h"
#include "core/render/bl_box.h"
#include "core/render/circuit/alpha_values.h"
#include "core/render/context.h"
#include "core/render/context_guard.h"
#include "core/render/primitive/point.h"
#include "core/render/primitive/text.h"
#include "core/vocabulary/decoration_type.h"
#include "core/vocabulary/drawable_element.h"
#include "core/vocabulary/point_fine.h"

namespace logicsim {

namespace defaults {

constexpr static inline auto text_element_angle_color_empty = defaults::color_gray;
constexpr static inline auto text_element_angle_color_truncated = defaults::color_orange;

// values from (0 - 0.5] are allowed
constexpr static inline auto text_element_angle_size_empty = grid_fine_t {0.25};
constexpr static inline auto text_element_angle_size_truncated = grid_fine_t {0.4};

constexpr static inline auto text_element_font_size = grid_fine_t {0.8};

}  // namespace defaults

namespace {

/**
 * @brief: Draw an angle with the tip at origin.
 *
 * Note, shift determines the size and direction. Left for positive, right for negative.
 */
auto draw_decoration_text_angle_primitive(Context& ctx, point_fine_t origin, double shift,
                                          color_t color,
                                          TextTruncated truncated) -> void {
    const auto stroke_factor = truncated == TextTruncated::no ? 1 : 3;
    const auto stroke_width = ctx.view_config().stroke_width() * stroke_factor;

    const auto [x, y] = to_context(origin, ctx);

    const auto poly = std::array {
        BLPoint {x + shift, y - shift},
        BLPoint {x, y},
        BLPoint {x + shift, y + shift},
    };
    const auto view = BLArrayView<BLPoint> {poly.data(), poly.size()};

    const auto _ = ContextGuard {ctx.bl_ctx};
    ctx.bl_ctx.setStrokeStartCap(BL_STROKE_CAP_ROUND);
    ctx.bl_ctx.setStrokeEndCap(BL_STROKE_CAP_ROUND);
    ctx.bl_ctx.setStrokeWidth(stroke_width);
    ctx.bl_ctx.strokePolyline(BLArrayView<BLPoint>(view), color);
}

/**
 * @brief: Offset of the angles origin of the text element from the position.
 */
auto text_element_angle_offset(grid_fine_t angle_size) -> point_fine_t {
    Expects(angle_size > grid_fine_t {0});
    Expects(angle_size <= grid_fine_t {0.5});

    return point_fine_t {grid_fine_t {0.25} + angle_size / 2, 0};
}

enum class BracketType { open, close };

/**
 * @brief: Draw a single angle of the text element.
 */
auto draw_decoration_text_angle(Context& ctx, point_t position, size_2d_t size,
                                ElementDrawState state, TextTruncated truncated,
                                BracketType type) -> void {
    const auto color_base = truncated == TextTruncated::no
                                ? defaults::text_element_angle_color_empty
                                : defaults::text_element_angle_color_truncated;
    const auto angle_size = truncated == TextTruncated::no
                                ? defaults::text_element_angle_size_empty
                                : defaults::text_element_angle_size_truncated;

    const auto angle_offset = text_element_angle_offset(angle_size);
    const auto color = with_alpha_runtime(color_base, state);
    const auto shift = to_context(angle_size, ctx);

    if (type == BracketType::open) {
        const auto origin_start = position - angle_offset;
        draw_decoration_text_angle_primitive(ctx, origin_start, shift, color, truncated);
    }

    else if (type == BracketType::close) {
        const auto position_end = point_t {to_grid(size.width, position.x), position.y};
        const auto origin_end = position_end + angle_offset;
        draw_decoration_text_angle_primitive(ctx, origin_end, -shift, color, truncated);
    }
}

/**
 * @brief: Draw all angles of the text decoration
 */
auto draw_decoration_text_angles(Context& ctx, point_t position, size_2d_t size,
                                 ElementDrawState state,
                                 const draw_text_result_t& draw_result) -> void {
    if (draw_result.truncated == TextTruncated::no &&
        !is_box_empty(draw_result.bounding_box)) {
        return;
    }

    for (auto offset : range_inclusive<grid_t>(0, int {size.height})) {
        const auto pos = position + point_t {0, offset};

        draw_decoration_text_angle(ctx, pos, size, state, TextTruncated::no,
                                   BracketType::open);
        draw_decoration_text_angle(ctx, pos, size, state, draw_result.truncated,
                                   BracketType::close);
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

    // text
    const auto text_anchor = point_fine_t {position.x, position.y}  //
                             + point_fine_t {0, int {size.height} / 2.0};
    const auto text_color = with_alpha_runtime(defaults::color_black, state);
    const auto font_size = (int {size.height} + 1) * defaults::text_element_font_size;
    const auto& text_label = layout.decorations().attrs_text_element(decoration_id).text;

    const auto draw_result =
        draw_text(ctx, text_anchor, text_label,
                  TextAttributes {
                      .font_size = font_size,
                      .color = text_color,

                      .horizontal_alignment = HTextAlignment::left,
                      .vertical_alignment = VTextAlignment::center_baseline,
                      .style = FontStyle::regular,

                      .max_text_width = grid_fine_t {int {size.width}},
                  });

    // angles
    draw_decoration_text_angles(ctx, position, size, state, draw_result);
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
