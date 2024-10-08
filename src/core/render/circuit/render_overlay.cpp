#include "render/circuit/render_overlay.h"

#include "concept/input_range.h"
#include "layout.h"
// #include "layout_info.h"
#include "layout_info.h"
#include "element/logicitem/render_logicitem_overlay.h"
#include "render/context.h"
#include "render/primitive/round_rect.h"
#include "vocabulary/color.h"
#include "vocabulary/layout_calculation_data.h"
#include "vocabulary/rect_fine.h"

namespace logicsim {

namespace defaults {
namespace overlay_color {
constexpr static inline auto selected = color_t {0, 128, 255, 96};
constexpr static inline auto valid = color_t {0, 192, 0, 96};
constexpr static inline auto colliding = color_t {255, 0, 0, 96};
}  // namespace overlay_color
}  // namespace defaults

template <>
auto format(shadow_t orientation) -> std::string {
    switch (orientation) {
        using enum shadow_t;

        case selected:
            return "selected";
        case valid:
            return "valid";
        case colliding:
            return "colliding";
    }
    throw std::runtime_error("Don't know how to convert shadow_t to string.");
}

auto shadow_color(shadow_t shadow_type) -> color_t {
    switch (shadow_type) {
        case shadow_t::selected: {
            return defaults::overlay_color::selected;
        }
        case shadow_t::valid: {
            return defaults::overlay_color::valid;
        }
        case shadow_t::colliding: {
            return defaults::overlay_color::colliding;
        }
    };

    throw std::runtime_error("unknown shadow type");
}

auto draw_logicitem_shadow(Context& ctx, const Layout& layout,
                            logicitem_id_t logicitem_id, shadow_t shadow_type) -> void {
    const auto layout_data = to_layout_calculation_data(layout, logicitem_id);
    const auto rect = element_shadow_rect(layout_data);

    draw_round_rect(ctx, rect,
                    {
                        .draw_type = ShapeDrawType::fill,
                        .rounding = element_shadow_rounding(layout_data.logicitem_type),
                        .fill_color = shadow_color(shadow_type),
                    });
}

auto draw_logicitem_shadows(Context& ctx, const Layout& layout,
                             std::span<const logicitem_id_t> elements,
                             shadow_t shadow_type) -> void {
    for (const auto& logicitem_id : elements) {
        draw_logicitem_shadow(ctx, layout, logicitem_id, shadow_type);
    }
}

template <input_range_of<ordered_line_t> View>
auto draw_wire_shadows_impl(Context& ctx, View lines, shadow_t shadow_type) -> void {
    const auto color = shadow_color(shadow_type);

    for (const ordered_line_t line : lines) {
        const auto selection_rect = element_shadow_rect(line);
        draw_round_rect(ctx, selection_rect,
                        {
                            .draw_type = ShapeDrawType::fill,
                            .stroke_width = defaults::use_view_config_stroke_width,
                            .fill_color = color,

                        });
    }
}

auto draw_wire_shadows(Context& ctx, std::span<const ordered_line_t> lines,
                       shadow_t shadow_type) -> void {
    draw_wire_shadows_impl(ctx, lines, shadow_type);
}

auto draw_wire_shadows(Context& ctx, std::span<const segment_info_t> segment_infos,
                       shadow_t shadow_type) -> void {
    draw_wire_shadows_impl(
        ctx, transform_view(segment_infos, [](segment_info_t info) { return info.line; }),
        shadow_type);
}

}  // namespace logicsim
