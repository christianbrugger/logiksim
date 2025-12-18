#include "core/render/circuit/render_logicitem_base_generic.h"

#include "core/format/std_type.h"
#include "core/geometry/rect.h"
#include "core/layout.h"
#include "core/layout_info.h"
#include "core/render/circuit/alpha_values.h"
#include "core/render/context.h"
#include "core/render/primitive/rect.h"
#include "core/render/primitive/text.h"
#include "core/vocabulary/layout_calculation_data.h"
#include "core/vocabulary/rect_fine.h"

#include <fmt/core.h>

namespace logicsim {

namespace defaults {
constexpr static inline auto body_stroke_color = defaults::color_black;

namespace body_fill_color {
constexpr static inline auto normal = color_t {255, 255, 128};
constexpr static inline auto normal_selected = color_t {224, 224, 224};
constexpr static inline auto valid = color_t {192, 192, 192};
constexpr static inline auto colliding = color_t {192, 192, 192};
constexpr static inline auto temporary_selected = color_t {192, 192, 192};
}  // namespace body_fill_color

namespace font {
constexpr static inline auto logicitem_label_color = defaults::color_black;
constexpr static inline auto logicitem_label_size = grid_fine_t {0.9};
constexpr static inline auto text_cutoff_px = 3.0;  // pixels
constexpr static inline auto binary_value_size = grid_fine_t {0.7};
}  // namespace font
}  // namespace defaults

auto get_logicitem_fill_color(ElementDrawState state) -> color_t {
    switch (state) {
        using enum ElementDrawState;
        using namespace defaults;

        case normal:
            return with_alpha(body_fill_color::normal, normal);
        case normal_selected:
            return with_alpha(body_fill_color::normal_selected, normal_selected);
        case valid:
            return with_alpha(body_fill_color::valid, valid);
        case simulated:
            return with_alpha(body_fill_color::normal, simulated);
        case colliding:
            return with_alpha(body_fill_color::colliding, colliding);
        case temporary_selected:
            return with_alpha(body_fill_color::temporary_selected, temporary_selected);
    };

    throw std::runtime_error("draw state has no logic item base color");
}

auto get_logicitem_stroke_color(ElementDrawState state) -> color_t {
    return with_alpha_runtime(defaults::body_stroke_color, state);
}

auto get_logicitem_label_color(ElementDrawState state) -> color_t {
    return with_alpha_runtime(defaults::font::logicitem_label_color, state);
}

auto get_logicitem_center(const Layout& layout, logicitem_id_t logicitem_id)
    -> point_fine_t {
    const auto layout_data = to_layout_calculation_data(layout, logicitem_id);
    const auto rect = element_body_draw_rect(layout_data);
    return get_center(rect);
}

auto LogicItemRectAttributes::format() const -> std::string {
    return fmt::format(
        "LogicItemRectAttributes(custom_fill_color = {}, custom_stroke_color = {})",
        custom_fill_color, custom_stroke_color);
}

auto draw_logicitem_rect(Context& ctx, rect_fine_t rect, ElementDrawState state,
                         LogicItemRectAttributes attributes)

    -> void {
    const auto fill_color = attributes.custom_fill_color
                                ? with_alpha_runtime(*attributes.custom_fill_color, state)
                                : get_logicitem_fill_color(state);
    const auto stroke_color =
        attributes.custom_stroke_color
            ? with_alpha_runtime(*attributes.custom_stroke_color, state)
            : get_logicitem_stroke_color(state);

    draw_rect(ctx, rect,
              RectAttributes {
                  .draw_type = ShapeDrawType::fill_and_stroke,
                  .fill_color = fill_color,
                  .stroke_color = stroke_color,
              });
}

auto draw_logicitem_rect(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                         ElementDrawState state, LogicItemRectAttributes attributes)
    -> void {
    const auto layout_data = to_layout_calculation_data(layout, logicitem_id);
    const auto rect = element_body_draw_rect(layout_data);
    draw_logicitem_rect(ctx, rect, state, attributes);
}

auto draw_logicitem_label(Context& ctx, point_fine_t center, std::string_view text,
                          ElementDrawState state, LogicItemTextAttributes attributes)
    -> void {
    if (text.empty()) {
        return;
    }

    const auto font_size = attributes.custom_font_size
                               ? *attributes.custom_font_size
                               : defaults::font::logicitem_label_size;

    const auto text_color = attributes.custom_text_color
                                ? with_alpha_runtime(*attributes.custom_text_color, state)
                                : get_logicitem_label_color(state);

    draw_text(ctx, center, text,
              TextAttributes {
                  .font_size = font_size,
                  .color = text_color,
                  .horizontal_alignment = attributes.horizontal_alignment,
                  .vertical_alignment = attributes.vertical_alignment,
                  .style = attributes.style,
                  .cutoff_size_px = defaults::font::text_cutoff_px,
              });
}

auto LogicItemTextAttributes::format() const -> std::string {
    return fmt::format(
        "LogicItemRectAttributes(custom_font_size = {}, custom_text_color = {}"
        "horizontal_alignment = {}, vertical_alignment = {}, style = {})",
        custom_font_size, custom_text_color, horizontal_alignment, vertical_alignment,
        style);
}

auto draw_logicitem_label(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                          std::string_view text, ElementDrawState state,
                          LogicItemTextAttributes attributes) -> void {
    const auto center = get_logicitem_center(layout, logicitem_id);
    draw_logicitem_label(ctx, center, text, state, attributes);
}

auto draw_binary_value(Context& ctx, point_fine_t point, bool is_enabled,
                       ElementDrawState state) -> void {
    const auto text = is_enabled ? std::string_view {"1"} : std::string_view {"0"};
    draw_logicitem_label(ctx, point, text, state,
                         LogicItemTextAttributes {
                             .custom_font_size = defaults::font::binary_value_size,
                         });
}

auto draw_binary_true(Context& ctx, point_fine_t point, ElementDrawState state) -> void {
    const auto is_enabled = true;
    draw_binary_value(ctx, point, is_enabled, state);
}

auto draw_binary_false(Context& ctx, point_fine_t point, ElementDrawState state) -> void {
    const auto is_enabled = false;
    draw_binary_value(ctx, point, is_enabled, state);
}

}  // namespace logicsim
