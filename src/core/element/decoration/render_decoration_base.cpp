#include "core/element/decoration/render_decoration_base.h"

#include "core/algorithm/range_extended.h"
#include "core/format/enum.h"
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

namespace text_element {

constexpr static inline auto angle_color_regular = defaults::color_light_gray;
constexpr static inline auto angle_color_empty = defaults::color_gray;
constexpr static inline auto angle_color_truncated = defaults::color_orange;

// values from (0 - 0.5] are allowed
constexpr static inline auto angle_size_regular = grid_fine_t {0.20};
constexpr static inline auto angle_size_empty = grid_fine_t {0.25};
constexpr static inline auto angle_size_truncated = grid_fine_t {0.4};

constexpr static inline auto angle_stroke_factor_regular = 1;
constexpr static inline auto angle_stroke_factor_empty = 1;
constexpr static inline auto angle_stroke_factor_truncated = 3;

constexpr static inline auto font_size = grid_fine_t {0.8};
}  // namespace text_element

}  // namespace defaults

enum class TextElementState {
    regular,
    empty,
    truncated,
};

template <>
[[nodiscard]] auto format(TextElementState value) -> std::string;

template <>
auto format(TextElementState value) -> std::string {
    switch (value) {
        using enum TextElementState;

        case regular: {
            return "regular";
        }
        case empty: {
            return "empty";
        }
        case truncated: {
            return "truncated";
        }
    };
    std::terminate();
}

namespace {

/**
 * @brief: Draw an angle with the tip at origin.
 *
 * Note, shift determines the size and direction. Left for positive, right for negative.
 */
auto draw_decoration_text_angle_primitive(Context& ctx, point_fine_t origin, double shift,
                                          color_t color, int stroke_factor) -> void {
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

[[nodiscard]] auto to_bracked_base_color(TextElementState state,
                                         BracketType type) -> color_t {
    switch (state) {
        using enum TextElementState;

        case regular: {
            return defaults::text_element::angle_color_regular;
        }
        case truncated: {
            if (type == BracketType::open) {
                return defaults::text_element::angle_color_empty;
            }
            return defaults::text_element::angle_color_truncated;
        }
        case empty: {
            return defaults::text_element::angle_color_empty;
        }
    }
    std::terminate();
}

[[nodiscard]] auto to_angle_size(TextElementState state,
                                 BracketType type) -> grid_fine_t {
    switch (state) {
        using enum TextElementState;

        case regular: {
            return defaults::text_element::angle_size_regular;
        }
        case truncated: {
            if (type == BracketType::open) {
                return defaults::text_element::angle_size_empty;
            }
            return defaults::text_element::angle_size_truncated;
        }
        case empty: {
            return defaults::text_element::angle_size_empty;
        }
    }
    std::terminate();
}

[[nodiscard]] auto to_stroke_factor(TextElementState state, BracketType type) -> int {
    switch (state) {
        using enum TextElementState;

        case regular: {
            return defaults::text_element::angle_stroke_factor_regular;
        }
        case truncated: {
            if (type == BracketType::open) {
                return defaults::text_element::angle_stroke_factor_empty;
            }
            return defaults::text_element::angle_stroke_factor_truncated;
        }
        case empty: {
            return defaults::text_element::angle_stroke_factor_empty;
        }
    }
    std::terminate();
}

/**
 * @brief: Draw a single angle of the text element.
 */
auto draw_decoration_text_angle(Context& ctx, point_t position, size_2d_t size,
                                ElementDrawState draw_state, TextElementState text_state,
                                BracketType type) -> void {
    const auto color_base = to_bracked_base_color(text_state, type);
    const auto angle_size = to_angle_size(text_state, type);
    const auto stroke_factor = to_stroke_factor(text_state, type);

    const auto angle_offset = text_element_angle_offset(angle_size);
    const auto color = with_alpha_runtime(color_base, draw_state);
    const auto shift = to_context(angle_size, ctx);

    if (type == BracketType::open) {
        const auto origin_start = position - angle_offset;
        draw_decoration_text_angle_primitive(ctx, origin_start, shift, color,
                                             stroke_factor);
    }

    else if (type == BracketType::close) {
        const auto position_end = point_t {to_grid(size.width, position.x), position.y};
        const auto origin_end = position_end + angle_offset;
        draw_decoration_text_angle_primitive(ctx, origin_end, -shift, color,
                                             stroke_factor);
    }
}

/**
 * @brief: Draw all angles of the text decoration
 */
auto draw_decoration_text_angles(Context& ctx, point_t position, size_2d_t size,
                                 ElementDrawState draw_state,
                                 TextElementState text_state) -> void {
    if (draw_state == ElementDrawState::simulated &&
        text_state != TextElementState::truncated) {
        return;
    }

    for (auto offset : range_inclusive<grid_t>(0, int {size.height})) {
        const auto pos = position + point_t {0, offset};

        draw_decoration_text_angle(ctx, pos, size, draw_state, text_state,
                                   BracketType::open);
        draw_decoration_text_angle(ctx, pos, size, draw_state, text_state,
                                   BracketType::close);
    }
}

[[nodiscard]] auto to_text_element_state(draw_text_result_t draw_result)
    -> TextElementState {
    if (draw_result.truncated == TextTruncated::yes) {
        return TextElementState::truncated;
    }
    if (is_box_empty(draw_result.bounding_box)) {
        return TextElementState::empty;
    }
    return TextElementState::regular;
}

auto text_element_text_anchor(point_t position, size_2d_t size,
                              HTextAlignment horizontal_alignment) -> point_fine_t {
    const auto x_offset = [&]() -> double {
        switch (horizontal_alignment) {
            case HTextAlignment::center: {
                return int {size.width} / 2.0;
            }
            case HTextAlignment::left: {
                return 0;
            }
            case HTextAlignment::right: {
                return int {size.width};
            }
        }
        std::terminate();
    }();
    const auto y_offset = int {size.height} / 2.0;

    return position + point_fine_t {x_offset, y_offset};
}

/**
 * @brief: Draw the full text element.
 */
auto draw_decoration_text_element(Context& ctx, const Layout& layout,
                                  decoration_id_t decoration_id,
                                  ElementDrawState draw_state) -> void {
    const auto position = layout.decorations().position(decoration_id);
    const auto size = layout.decorations().size(decoration_id);
    const auto& attrs = layout.decorations().attrs_text_element(decoration_id);

    // text
    const auto text_anchor =
        text_element_text_anchor(position, size, attrs.horizontal_alignment);
    const auto text_color = with_alpha_runtime(attrs.text_color, draw_state);
    const auto font_size = (int {size.height} + 1) * defaults::text_element::font_size;

    const auto draw_result =
        draw_text(ctx, text_anchor, attrs.text,
                  TextAttributes {
                      .font_size = font_size,
                      .color = text_color,

                      .horizontal_alignment = attrs.horizontal_alignment,
                      .vertical_alignment = VTextAlignment::center_baseline,
                      .style = attrs.font_style,

                      .max_text_width = grid_fine_t {int {size.width}},
                  });

    // angles
    const auto text_state = to_text_element_state(draw_result);
    draw_decoration_text_angles(ctx, position, size, draw_state, text_state);
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
