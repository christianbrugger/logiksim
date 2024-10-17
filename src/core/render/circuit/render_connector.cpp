#include "core/render/circuit/render_connector.h"

#include "core/geometry/layout_calculation.h"
#include "core/geometry/orientation.h"
#include "core/layout.h"
#include "core/layout_info.h"
#include "core/render/circuit/alpha_values.h"
#include "core/render/circuit/render_wire.h"
#include "core/render/context.h"
#include "core/render/primitive/line.h"
#include "core/render/primitive/stroke.h"
#include "core/spatial_simulation.h"
#include "core/vocabulary/color.h"
#include "core/vocabulary/drawable_element.h"
#include "core/vocabulary/layout_calculation_data.h"
#include "core/vocabulary/line_fine.h"

namespace logicsim {

namespace defaults {

constexpr static inline auto connector_cutoff_px = 3.0;  // pixels
constexpr static inline auto connector_length = grid_fine_t {0.4};

constexpr static inline auto inverted_circle_radius = grid_fine_t {0.2};
constexpr static inline auto inverted_connector_fill = defaults::color_white;
}  // namespace defaults

// TODO: rename do sounds like doing it
auto do_draw_connector(const ViewConfig& view_config) {
    return view_config.pixel_scale() >= defaults::connector_cutoff_px;
}

auto inverter_stroke_width_px(bool is_enabled, WireRenderStyle style,
                              int view_stroke_width_px) -> int {
    switch (style) {
        case WireRenderStyle::red:
            return view_stroke_width_px;
        case WireRenderStyle::bold:
            return is_enabled ? view_stroke_width_px * 3 : view_stroke_width_px;
        case WireRenderStyle::bold_red:
            return is_enabled ? view_stroke_width_px * 2 : view_stroke_width_px;
    };
    std::terminate();
}

auto _draw_connector_inverted(Context& ctx, ConnectorAttributes attributes) {
    const auto radius = defaults::inverted_circle_radius;
    const auto width =
        inverter_stroke_width_px(attributes.is_enabled, ctx.settings.wire_render_style,
                                 ctx.view_config().stroke_width());
    const auto offset = stroke_offset(width);

    const auto r = to_context_unrounded(radius, ctx);
    const auto p = to_context(attributes.position, ctx);
    const auto p_center = connector_point(p, attributes.orientation, r + width / 2.0);
    const auto p_adjusted = is_horizontal(attributes.orientation)
                                ? BLPoint {p_center.x, p_center.y + offset}
                                : BLPoint {p_center.x + offset, p_center.y};

    const auto fill_color =
        with_alpha_runtime(defaults::inverted_connector_fill, attributes.state);
    const auto stroke_color = wire_color(
        attributes.is_enabled, ctx.settings.wire_render_style, attributes.state);

    ctx.bl_ctx.fillCircle(BLCircle {p_adjusted.x, p_adjusted.y, r + width / 2.0},
                          stroke_color);
    ctx.bl_ctx.fillCircle(BLCircle {p_adjusted.x, p_adjusted.y, r - width / 2.0},
                          fill_color);
}

auto _draw_connector_normal(Context& ctx, ConnectorAttributes attributes) -> void {
    const auto endpoint = connector_point(attributes.position, attributes.orientation,
                                          defaults::connector_length);

    const auto color = wire_color(attributes.is_enabled, ctx.settings.wire_render_style,
                                  attributes.state);
    const auto stroke_width =
        wire_stroke_width_px(attributes.is_enabled, ctx.settings.wire_render_style,
                             ctx.view_config().stroke_width());

    draw_line(ctx, line_fine_t {attributes.position, endpoint},
              {.color = color, .stroke_width = stroke_width});
}

auto draw_connector(Context& ctx, ConnectorAttributes attributes) -> void {
    if (attributes.orientation == orientation_t::undirected) {
        return;
    }

    if (attributes.is_inverted) {
        _draw_connector_inverted(ctx, attributes);
    } else {
        _draw_connector_normal(ctx, attributes);
    }
}

auto draw_logicitem_connectors(Context& ctx, const Layout& layout,
                               logicitem_id_t logicitem_id,
                               ElementDrawState state) -> void {
    const auto layout_data = to_layout_calculation_data(layout, logicitem_id);

    for (auto info : input_locations_and_id(layout_data)) {
        draw_connector(ctx, ConnectorAttributes {
                                .state = state,
                                .position = info.position,
                                .orientation = info.orientation,
                                .is_inverted = layout.logicitems().input_inverted(
                                    logicitem_id, info.input_id),
                                .is_enabled = false,
                            });
    }

    for (auto info : output_locations_and_id(layout_data)) {
        draw_connector(ctx, ConnectorAttributes {
                                .state = state,
                                .position = info.position,
                                .orientation = info.orientation,
                                .is_inverted = layout.logicitems().output_inverted(
                                    logicitem_id, info.output_id),
                                .is_enabled = false,
                            });
    }
}

auto draw_logicitem_connectors(Context& ctx, const SpatialSimulation& spatial_simulation,
                               logicitem_id_t logicitem_id) -> void {
    const auto& logicitems = spatial_simulation.layout().logicitems();

    const auto element_id = to_element_id(spatial_simulation, logicitem_id);
    const auto layout_data = to_layout_calculation_data(logicitems, logicitem_id);

    for (auto info : input_locations_and_id(layout_data)) {
        const auto input = input_t {element_id, info.input_id};

        const auto is_inverted = logicitems.input_inverted(logicitem_id, info.input_id);
        const auto is_connected =
            is_input_connected(spatial_simulation.schematic(), input);
        const auto is_enabled = spatial_simulation.simulation().input_value(input);

        if (is_inverted || !is_connected) {
            draw_connector(ctx, ConnectorAttributes {
                                    .state = ElementDrawState::normal,
                                    .position = info.position,
                                    .orientation = info.orientation,
                                    .is_inverted = is_inverted,
                                    .is_enabled = is_enabled,
                                });
        }
    }

    for (auto info : output_locations_and_id(layout_data)) {
        const auto output = output_t {element_id, info.output_id};

        const auto is_inverted = logicitems.output_inverted(logicitem_id, info.output_id);
        const auto is_connected =
            is_output_connected(spatial_simulation.schematic(), output);
        const auto is_enabled =
            spatial_simulation.simulation().output_value(output).value();

        if (is_inverted || !is_connected) {
            draw_connector(ctx, ConnectorAttributes {
                                    .state = ElementDrawState::normal,
                                    .position = info.position,
                                    .orientation = info.orientation,
                                    .is_inverted = is_inverted,
                                    .is_enabled = is_enabled,
                                });
        }
    }
}

auto draw_logicitems_connectors(Context& ctx, const Layout& layout,
                                std::span<const DrawableLogicItem> elements) -> void {
    if (do_draw_connector(ctx.view_config())) {
        for (const auto entry : elements) {
            draw_logicitem_connectors(ctx, layout, entry.logicitem_id, entry.state);
        }
    }
}

auto draw_logicitems_connectors(Context& ctx, const SpatialSimulation& spatial_simulation,
                                std::span<const logicitem_id_t> elements) -> void {
    if (do_draw_connector(ctx.view_config())) {
        for (const auto logicitem_id : elements) {
            draw_logicitem_connectors(ctx, spatial_simulation, logicitem_id);
        }
    }
}

}  // namespace logicsim
