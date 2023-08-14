#include "render_circuit.h"

#include "collision.h"
#include "concept.h"
#include "editable_circuit/selection.h"
#include "layout.h"
#include "layout_calculation.h"
#include "timer.h"

#include <numbers>

namespace logicsim {

//
// Logic Items Body
//

auto draw_logic_item_above(ElementType type) -> bool {
    return type == ElementType::button;
}

auto get_logic_item_state(layout::ConstElement element, const Selection* selection)
    -> ElementDrawState {
    const auto is_selected = [&]() {
        return (selection != nullptr) ? selection->is_selected(element.element_id())
                                      : false;
    };

    const auto display_state = element.display_state();

    if (is_inserted(display_state)) {
        if (display_state == display_state_t::valid) {
            return ElementDrawState::valid;
        } else if (is_selected()) {
            return ElementDrawState::normal_selected;
        }
        return ElementDrawState::normal;
    } else {
        if (display_state == display_state_t::colliding) {
            return ElementDrawState::colliding;
        } else if (is_selected()) {
            return ElementDrawState::temporary_selected;
        }
        throw_exception("cannot draw temporary items");
    }
}

auto get_logic_item_fill_color(ElementDrawState state) -> color_t {
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

    throw_exception("draw state has no logic item base color");
}

auto get_logic_item_stroke_color(ElementDrawState state) -> color_t {
    return with_alpha_runtime(defaults::body_stroke_color, state);
}

auto get_logic_item_text_color(ElementDrawState state) -> color_t {
    return with_alpha_runtime(defaults::font::logic_item_text_color, state);
}

auto draw_logic_item_rect(BLContext& ctx, rect_fine_t rect, layout::ConstElement element,
                          ElementDrawState state, const RenderSettings& settings,
                          LogicItemRectAttributes attributes) -> void {
    const auto final_rect = rect + point_fine_t {element.position()};

    const auto fill_color = attributes.custom_fill_color
                                ? with_alpha_runtime(*attributes.custom_fill_color, state)
                                : get_logic_item_fill_color(state);
    const auto stroke_color =
        attributes.custom_stroke_color
            ? with_alpha_runtime(*attributes.custom_stroke_color, state)
            : get_logic_item_stroke_color(state);

    draw_rect(ctx, final_rect,
              RectAttributes {
                  .draw_type = DrawType::fill_and_stroke,
                  .fill_color = fill_color,
                  .stroke_color = stroke_color,
              },
              settings);
}

auto draw_logic_item_label(BLContext& ctx, point_fine_t point, const std::string& text,
                           layout::ConstElement element, ElementDrawState state,
                           const RenderSettings& settings,
                           LogicItemTextAttributes attributes) -> void {
    const auto center = point + point_fine_t {element.position()};

    const auto font_size = attributes.custom_font_size
                               ? *attributes.custom_font_size
                               : defaults::font::logic_item_label_size;

    const auto fill_color = attributes.custom_fill_color
                                ? with_alpha_runtime(*attributes.custom_fill_color, state)
                                : get_logic_item_text_color(state);

    draw_text(ctx, center, text,
              TextAttributes {
                  .font_size = font_size,
                  .fill_color = fill_color,
                  .horizontal_alignment = HorizontalAlignment::center,
                  .vertical_alignment = VerticalAlignment::center,
                  .cuttoff_size_px = defaults::font::text_cutoff_px,
              },
              settings);
}

auto draw_binary_value(BLContext& ctx, point_fine_t point, bool is_enabled,
                       layout::ConstElement element, ElementDrawState state,
                       const RenderSettings& settings) -> void {
    const auto text = is_enabled ? std::string {"1"} : std::string {"0"};
    draw_logic_item_label(ctx, point, text, element, state, settings,
                          LogicItemTextAttributes {
                              .custom_font_size = defaults::font::binary_value_size,
                          });
}

auto draw_binary_false(BLContext& ctx, point_fine_t point, layout::ConstElement element,
                       ElementDrawState state, const RenderSettings& settings) -> void {
    const auto is_enabled = false;
    draw_binary_value(ctx, point, is_enabled, element, state, settings);
}

auto _draw_connector_inverted(BLContext& ctx, ConnectorAttributes attributes,
                              const RenderSettings& settings) {
    const auto radius = defaults::inverted_circle_radius;
    const auto width = settings.view_config.stroke_width();
    const auto offset = stroke_offset(width);

    const auto r = radius * settings.view_config.pixel_scale();
    const auto p = to_context(attributes.position, settings.view_config);
    const auto p_center = connector_point(p, attributes.orientation, r + width);

    const auto fill_color =
        with_alpha_runtime(defaults::inverted_connector_fill, attributes.state);
    const auto stroke_color = wire_color(attributes.is_enabled, attributes.state);

    ctx.setFillStyle(fill_color);
    ctx.fillCircle(BLCircle {p_center.x + offset, p_center.y + offset, r});

    ctx.setStrokeWidth(width);
    ctx.setStrokeStyle(stroke_color);
    ctx.strokeCircle(BLCircle {p_center.x + offset, p_center.y + offset, r});
}

auto _draw_connector_normal(BLContext& ctx, ConnectorAttributes attributes,
                            const RenderSettings& settings) -> void {
    const auto endpoint = connector_point(attributes.position, attributes.orientation,
                                          defaults::connector_length);
    draw_line(ctx, line_fine_t {attributes.position, endpoint},
              {.color = wire_color(attributes.is_enabled, attributes.state)}, settings);
}

auto draw_connector(BLContext& ctx, ConnectorAttributes attributes,
                    const RenderSettings& settings) -> void {
    if (attributes.orientation == orientation_t::undirected) {
        return;
    }

    if (attributes.is_inverted) {
        _draw_connector_inverted(ctx, attributes, settings);
    } else {
        _draw_connector_normal(ctx, attributes, settings);
    }
}

//
// Standard Element
//

auto standard_element_label(layout::ConstElement element) -> std::string {
    switch (element.element_type()) {
        using enum ElementType;

        case and_element:
            return "&";
        case or_element:
            return ">1";
        case xor_element:
            return "=1";

        default:
            return "";
    }
}

auto draw_standard_element(BLContext& ctx, layout::ConstElement element,
                           ElementDrawState state, const RenderSettings& settings)
    -> void {
    const auto element_height =
        std::max(element.input_count(), element.output_count()) - 1;
    const auto padding = defaults::logic_item_body_overdraw;
    const auto rect = rect_fine_t {
        point_fine_t {0., -padding},
        point_fine_t {2., element_height + padding},
    };

    draw_logic_item_rect(ctx, rect, element, state, settings);
    draw_logic_item_label(ctx, point_fine_t {1., element_height / 2.0},
                          standard_element_label(element), element, state, settings);
}

auto draw_button(BLContext& ctx, layout::ConstElement element, ElementDrawState state,
                 const RenderSettings& settings) -> void {
    const auto padding = defaults::button_body_overdraw;
    const auto rect = rect_fine_t {
        point_fine_t {-padding, -padding},
        point_fine_t {+padding, +padding},
    };

    draw_logic_item_rect(ctx, rect, element, state, settings,
                         {.custom_fill_color = defaults::button_body_color});
    draw_binary_false(ctx, point_fine_t {0, 0}, element, state, settings);
}

auto draw_buffer(BLContext& ctx, layout::ConstElement element, ElementDrawState state,
                 const RenderSettings& settings) -> void {
    const auto padding = defaults::logic_item_body_overdraw;
    const auto rect = rect_fine_t {
        point_fine_t {0., -padding},
        point_fine_t {1., +padding},
    };

    draw_logic_item_rect(ctx, rect, element, state, settings);
    draw_logic_item_label(ctx, point_fine_t {0.5, 0.}, "1", element, state, settings,
                          {.custom_font_size = defaults::font::buffer_label_size});
}

auto draw_clock_generator(BLContext& ctx, layout::ConstElement element,
                          ElementDrawState state, const RenderSettings& settings)
    -> void {
    const auto padding = defaults::logic_item_body_overdraw;
    const auto rect = rect_fine_t {
        point_fine_t {0., 0. - padding},
        point_fine_t {3., 2. + padding},
    };
    draw_logic_item_rect(ctx, rect, element, state, settings);
}

auto draw_flipflop_jk(BLContext& ctx, layout::ConstElement element,
                      ElementDrawState state, const RenderSettings& settings) -> void {
    const auto padding = defaults::logic_item_body_overdraw;
    const auto rect = rect_fine_t {
        point_fine_t {0., 0. - padding},
        point_fine_t {4., 2. + padding},
    };

    draw_logic_item_rect(ctx, rect, element, state, settings);
    draw_logic_item_label(ctx, point_fine_t {2., 1.}, "JK-FF", element, state, settings);
}

auto draw_shift_register(BLContext& ctx, layout::ConstElement element,
                         ElementDrawState state, const RenderSettings& settings) -> void {
    const auto padding = defaults::logic_item_body_overdraw;
    const auto rect = rect_fine_t {
        point_fine_t {0., 0. - padding},
        point_fine_t {8., 2. + padding},
    };
    draw_logic_item_rect(ctx, rect, element, state, settings);

    // content
    const auto output_count = element.output_count();
    const auto state_size = std::size_t {10};

    for (auto n : range(output_count, state_size)) {
        const auto point = point_fine_t {
            -1 + 2.0 * (n / output_count),
            0.25 + 1.5 * (n % output_count),
        };
        draw_binary_false(ctx, point, element, state, settings);
    }
}

auto draw_latch_d(BLContext& ctx, layout::ConstElement element, ElementDrawState state,
                  const RenderSettings& settings) -> void {
    const auto padding = defaults::logic_item_body_overdraw;
    const auto rect = rect_fine_t {
        point_fine_t {0., 0. - padding},
        point_fine_t {2., 1. + padding},
    };

    draw_logic_item_rect(ctx, rect, element, state, settings);
    draw_logic_item_label(ctx, point_fine_t {1., 0.5}, "L", element, state, settings);
}

auto draw_flipflop_d(BLContext& ctx, layout::ConstElement element, ElementDrawState state,
                     const RenderSettings& settings) -> void {
    const auto padding = defaults::logic_item_body_overdraw;
    const auto rect = rect_fine_t {
        point_fine_t {0., 0. - padding},
        point_fine_t {3., 2. + padding},
    };

    draw_logic_item_rect(ctx, rect, element, state, settings);
    draw_logic_item_label(ctx, point_fine_t {1.5, 1.}, "FF", element, state, settings);
}

auto draw_flipflop_ms_d(BLContext& ctx, layout::ConstElement element,
                        ElementDrawState state, const RenderSettings& settings) -> void {
    const auto padding = defaults::logic_item_body_overdraw;
    const auto rect = rect_fine_t {
        point_fine_t {0., 0. - padding},
        point_fine_t {4., 2. + padding},
    };

    draw_logic_item_rect(ctx, rect, element, state, settings);
    draw_logic_item_label(ctx, point_fine_t {2., 1.}, "MS-FF", element, state, settings);
}

auto draw_logic_item_base(BLContext& ctx, layout::ConstElement element,
                          ElementDrawState state, const RenderSettings& settings)
    -> void {
    switch (element.element_type()) {
        using enum ElementType;

        case unused:
        case placeholder:
        case wire:
            throw_exception("not supported");

        case buffer_element:
            return draw_buffer(ctx, element, state, settings);

        case and_element:
        case or_element:
        case xor_element:
            return draw_standard_element(ctx, element, state, settings);

        case button:
            return draw_button(ctx, element, state, settings);

        case clock_generator:
            return draw_clock_generator(ctx, element, state, settings);
        case flipflop_jk:
            return draw_flipflop_jk(ctx, element, state, settings);
        case shift_register:
            return draw_shift_register(ctx, element, state, settings);
        case latch_d:
            return draw_latch_d(ctx, element, state, settings);
        case flipflop_d:
            return draw_flipflop_d(ctx, element, state, settings);
        case flipflop_ms_d:
            return draw_flipflop_ms_d(ctx, element, state, settings);

        case sub_circuit:
            return draw_standard_element(ctx, element, state, settings);
    }
    throw_exception("not supported");
}

auto draw_logic_item_connectors(BLContext& ctx, layout::ConstElement element,
                                ElementDrawState state, const RenderSettings& settings)
    -> void {
    const auto layout_data = to_layout_calculation_data(element.layout(), element);

    iter_input_location_and_id(
        layout_data,
        [&](connection_id_t input_id, point_t position, orientation_t orientation) {
            draw_connector(ctx,
                           ConnectorAttributes {
                               .state = state,
                               .position = position,
                               .orientation = orientation,
                               .is_inverted = element.input_inverted(input_id),
                               .is_enabled = false,
                           },
                           settings);
            return true;
        });

    iter_output_location_and_id(
        layout_data,
        [&](connection_id_t output_id, point_t position, orientation_t orientation) {
            draw_connector(ctx,
                           ConnectorAttributes {
                               .state = state,
                               .position = position,
                               .orientation = orientation,
                               .is_inverted = element.output_inverted(output_id),
                               .is_enabled = false,
                           },
                           settings);
            return true;
        });
}

auto draw_logic_items_base(BLContext& ctx, const Layout& layout,
                           std::span<const DrawableElement> elements,
                           const RenderSettings& settings) -> void {
    for (const auto entry : elements) {
        draw_logic_item_base(ctx, layout.element(entry.element_id), entry.state,
                             settings);
    }
}

auto draw_logic_items_connectors(BLContext& ctx, const Layout& layout,
                                 std::span<const DrawableElement> elements,
                                 const RenderSettings& settings) -> void {
    for (const auto entry : elements) {
        draw_logic_item_connectors(ctx, layout.element(entry.element_id), entry.state,
                                   settings);
    }
}

//
// Wires
//

auto wire_color(bool is_enabled, ElementDrawState state) -> color_t {
    if (is_enabled) {
        return with_alpha_runtime(defaults::wire_color_enabled, state);
    }
    return with_alpha_runtime(defaults::wire_color_disabled, state);
}

auto draw_line_cross_point(BLContext& ctx, const point_t point, bool is_enabled,
                           ElementDrawState state, const RenderSettings& settings)
    -> void {
    int lc_width = settings.view_config.line_cross_width();
    if (lc_width <= 0) {
        return;
    }

    const int wire_width = settings.view_config.stroke_width();
    const int wire_offset = (wire_width - 1) / 2;

    const int size = 2 * lc_width + wire_width;
    const int offset = wire_offset + lc_width;

    const auto [x, y] = to_context(point, settings.view_config);
    const auto color = wire_color(is_enabled, state);

    ctx.setFillStyle(color);
    ctx.fillRect(x - offset, y - offset, size, size);
}

auto draw_line_segment(BLContext& ctx, line_t line, bool is_enabled,
                       ElementDrawState state, const RenderSettings& settings) -> void {
    draw_line_segment(ctx, line_fine_t {line}, is_enabled, state, settings);

    draw_point(ctx, line.p0, PointShape::circle, defaults::color_orange, 0.2, settings);
    draw_point(ctx, line.p1, PointShape::cross, defaults::color_orange, 0.2, settings);
}

auto draw_line_segment(BLContext& ctx, line_fine_t line, bool is_enabled,
                       ElementDrawState state, const RenderSettings& settings) -> void {
    const auto color = wire_color(is_enabled, state);
    draw_line(ctx, line, {.color = color}, settings);
}

auto draw_line_segment(BLContext& ctx, segment_info_t info, bool is_enabled,
                       ElementDrawState state, const RenderSettings& settings) -> void {
    draw_line_segment(ctx, info.line, is_enabled, state, settings);

    if (is_cross_point(info.p0_type)) {
        draw_line_cross_point(ctx, info.line.p0, is_enabled, state, settings);
    }
    if (is_cross_point(info.p1_type)) {
        draw_line_cross_point(ctx, info.line.p1, is_enabled, state, settings);
    }
}

auto draw_segment_tree(BLContext& ctx, layout::ConstElement element,
                       ElementDrawState state, const RenderSettings& settings) -> void {
    const auto is_enabled = false;

    for (const segment_info_t& info : element.segment_tree().segment_infos()) {
        draw_line_segment(ctx, info, is_enabled, state, settings);
    }
}

//
//
//

auto draw_wires(BLContext& ctx, const Layout& layout,
                std::span<const DrawableElement> elements, const RenderSettings& settings)
    -> void {
    for (const auto entry : elements) {
        draw_segment_tree(ctx, layout.element(entry.element_id), entry.state, settings);
    }
}

auto draw_wires(BLContext& ctx, const Layout& layout,
                std::span<const element_id_t> elements, ElementDrawState state,
                const RenderSettings& settings) -> void {
    for (const auto element_id : elements) {
        draw_segment_tree(ctx, layout.element(element_id), state, settings);
    }
}

auto draw_wires(BLContext& ctx, std::span<const segment_info_t> segment_infos,
                ElementDrawState state, const RenderSettings& settings) -> void {
    const auto is_enabled = false;

    for (const auto& info : segment_infos) {
        draw_line_segment(ctx, info, is_enabled, state, settings);
    }
}

//
// Overlay
//

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
    throw_exception("Don't know how to convert shadow_t to string.");
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

    throw_exception("unknown shadow type");
}

auto element_shadow_rounding(ElementType type [[maybe_unused]]) -> double {
    return type == ElementType::button ? 0. : defaults::line_selection_padding;
}

auto draw_logic_item_shadow(BLContext& ctx, layout::ConstElement element,
                            shadow_t shadow_type, const RenderSettings& settings)
    -> void {
    const auto data = to_layout_calculation_data(element.layout(), element);
    const auto selection_rect = element_selection_rect(data);

    ctx.setFillStyle(shadow_color(shadow_type));
    draw_round_rect(ctx, selection_rect,
                    {
                        .draw_type = DrawType::fill,
                        .rounding = element_shadow_rounding(data.element_type),
                    },
                    settings);
}

auto draw_logic_item_shadows(BLContext& ctx, const Layout& layout,
                             std::span<const element_id_t> elements, shadow_t shadow_type,
                             const RenderSettings& settings) -> void {
    for (const auto element_id : elements) {
        draw_logic_item_shadow(ctx, layout.element(element_id), shadow_type, settings);
    }
}

template <input_range_of<ordered_line_t> View>
auto draw_wire_shadows_impl(BLContext& ctx, View lines, shadow_t shadow_type,
                            const RenderSettings& settings) -> void {
    const auto color = shadow_color(shadow_type);
    ctx.setFillStyle(BLRgba32(color.value));

    for (const ordered_line_t line : lines) {
        const auto selection_rect = element_selection_rect_rounded(line);
        draw_round_rect(ctx, selection_rect, {.draw_type = DrawType::fill}, settings);
    }
}

auto draw_wire_shadows(BLContext& ctx, std::span<const ordered_line_t> lines,
                       shadow_t shadow_type, const RenderSettings& settings) -> void {
    draw_wire_shadows_impl(ctx, lines, shadow_type, settings);
}

auto draw_wire_shadows(BLContext& ctx, std::span<const segment_info_t> segment_infos,
                       shadow_t shadow_type, const RenderSettings& settings) -> void {
    draw_wire_shadows_impl(
        ctx, transform_view(segment_infos, [](segment_info_t info) { return info.line; }),
        shadow_type, settings);
}

//
// Layout Rendering
//

auto render_inserted(BLContext& ctx, const Layout& layout,
                     const RenderSettings& settings) {
    const LayersCache& layers = settings.layers;

    ctx.setCompOp(BL_COMP_OP_SRC_COPY);

    draw_logic_items_base(ctx, layout, layers.normal_below, settings);
    draw_wires(ctx, layout, layers.normal_wires, ElementDrawState::normal, settings);
    draw_logic_items_base(ctx, layout, layers.normal_above, settings);

    draw_logic_items_connectors(ctx, layout, layers.normal_below, settings);
    draw_logic_items_connectors(ctx, layout, layers.normal_above, settings);
}

auto render_uninserted(BLContext& ctx, const Layout& layout,
                       const RenderSettings& settings) {
    const LayersCache& layers = settings.layers;

    if (settings.layer_surface_uninserted.enabled) {
        ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    } else {
        ctx.setCompOp(BL_COMP_OP_SRC_OVER);
    }

    draw_logic_items_base(ctx, layout, layers.uninserted_below, settings);
    draw_wires(ctx, layers.temporary_wires, ElementDrawState::temporary_selected,
               settings);
    draw_wires(ctx, layers.colliding_wires, ElementDrawState::colliding, settings);
    draw_logic_items_base(ctx, layout, layers.uninserted_above, settings);

    draw_logic_items_connectors(ctx, layout, layers.uninserted_below, settings);
    draw_logic_items_connectors(ctx, layout, layers.uninserted_above, settings);
}

auto render_overlay(BLContext& ctx, const Layout& layout, const RenderSettings& settings)
    -> void {
    const LayersCache& layers = settings.layers;

    if (settings.layer_surface_overlay.enabled) {
        ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    } else {
        ctx.setCompOp(BL_COMP_OP_SRC_OVER);
    }

    // selected & temporary
    draw_logic_item_shadows(ctx, layout, layers.selected_logic_items, shadow_t::selected,
                            settings);
    draw_wire_shadows(ctx, layers.selected_wires, shadow_t::selected, settings);
    draw_wire_shadows(ctx, layers.temporary_wires, shadow_t::selected, settings);

    // valid
    draw_logic_item_shadows(ctx, layout, layers.valid_logic_items, shadow_t::valid,
                            settings);
    draw_wire_shadows(ctx, layers.valid_wires, shadow_t::valid, settings);

    // colliding
    draw_logic_item_shadows(ctx, layout, layers.colliding_logic_items,
                            shadow_t::colliding, settings);
    draw_wire_shadows(ctx, layers.colliding_wires, shadow_t::colliding, settings);
}

auto render_layers(BLContext& ctx, const Layout& layout, const RenderSettings& settings)
    -> void {
    // TODO draw line inverters / connectors above wires
    // TODO draw uninserted wires in shadow

    if (settings.layers.has_inserted()) {
        render_inserted(ctx, layout, settings);
    }

    if (settings.layers.uninserted_bounding_rect.has_value()) {
        const auto rect = get_dirty_rect(settings.layers.uninserted_bounding_rect.value(),
                                         settings.view_config);

        render_to_layer(ctx, settings.layer_surface_uninserted, rect, settings,
                        [&](BLContext& layer_ctx) {
                            render_uninserted(layer_ctx, layout, settings);
                        });
    }

    if (settings.layers.overlay_bounding_rect.has_value()) {
        const auto rect = get_dirty_rect(settings.layers.overlay_bounding_rect.value(),
                                         settings.view_config);

        render_to_layer(
            ctx, settings.layer_surface_overlay, rect, settings,
            [&](BLContext& layer_ctx) { render_overlay(layer_ctx, layout, settings); });
    }
}

//
// Create Layers
//

auto add_valid_wire_parts(const layout::ConstElement wire,
                          std::vector<ordered_line_t>& output) -> bool {
    auto found = false;

    const auto& tree = wire.segment_tree();

    const auto& all_parts = tree.valid_parts();
    const auto begin = all_parts.begin();
    const auto end = all_parts.end();

    for (auto it = begin; it != end; ++it) {
        if (it->empty()) {
            continue;
        }
        const auto index =
            segment_index_t {gsl::narrow_cast<segment_index_t::value_type>(it - begin)};
        const auto full_line = tree.segment_line(index);

        for (const auto& part : *it) {
            output.push_back(to_line(full_line, part));
            found = true;
        }
    }

    return found;
}

auto add_selected_wire_parts(const layout::ConstElement wire, const Selection& selection,
                             std::vector<ordered_line_t>& output) -> void {
    const auto& tree = wire.segment_tree();

    for (const auto segment : tree.indices(wire.element_id())) {
        const auto parts = selection.selected_segments(segment);

        if (parts.empty()) {
            continue;
        }

        const auto full_line = tree.segment_line(segment.segment_index);

        for (const auto part : parts) {
            output.push_back(to_line(full_line, part));
        }
    }
}

auto insert_logic_item(LayersCache& layers, element_id_t element_id,
                       ElementType element_type, rect_t bounding_rect,
                       ElementDrawState state) -> void {
    if (is_inserted(state)) {
        if (draw_logic_item_above(element_type)) {
            layers.normal_above.push_back({element_id, state});
        } else {
            layers.normal_below.push_back({element_id, state});
        }
    } else {
        update_uninserted_rect(layers, bounding_rect);

        if (draw_logic_item_above(element_type)) {
            layers.uninserted_above.push_back({element_id, state});
        } else {
            layers.uninserted_below.push_back({element_id, state});
        }
    }

    if (has_overlay(state)) {
        update_overlay_rect(layers, bounding_rect);
    }

    switch (state) {
        using enum ElementDrawState;
        case normal:
        case simulated:
            break;

        case normal_selected:
        case temporary_selected:
            layers.selected_logic_items.push_back(element_id);
            break;
        case valid:
            layers.valid_logic_items.push_back(element_id);
            break;
        case colliding:
            layers.colliding_logic_items.push_back(element_id);
            break;
    }
}

auto build_layers(const Layout& layout, LayersCache& layers, const Selection* selection,
                  rect_t scene_rect) -> void {
    layers.clear();

    for (const auto element : layout.elements()) {
        // visibility
        const auto bounding_rect = element.bounding_rect();
        if (!is_colliding(bounding_rect, scene_rect)) {
            continue;
        }
        const auto element_type = element.element_type();

        if (is_logic_item(element_type)) {
            const auto state = get_logic_item_state(element, selection);
            insert_logic_item(layers, element, element_type, bounding_rect, state);
        }

        else if (element_type == ElementType::wire) {
            const auto display_state = element.display_state();

            if (is_inserted(display_state)) {
                layers.normal_wires.push_back(element);

                // TODO add: tree.has_valid_parts()
                const auto found_valid =
                    add_valid_wire_parts(element, layers.valid_wires);

                if (!found_valid && selection != nullptr) {
                    add_selected_wire_parts(element, *selection, layers.selected_wires);
                }
            } else {
                // fine grained check, as uninserted trees can contain a lot of segments
                for (const auto& info : element.segment_tree().segment_infos()) {
                    if (is_colliding(info.line, scene_rect)) {
                        // layers.uninserted_wires.push_back(info);
                        update_uninserted_rect(layers, info.line);

                        if (display_state == display_state_t::colliding) {
                            layers.colliding_wires.push_back(info);
                        } else if (display_state == display_state_t::temporary) {
                            layers.temporary_wires.push_back(info);
                        }
                    }
                }
            }
        }
    }

    layers.calculate_overlay_bounding_rect();
}

//
//
//

auto render_circuit_2(BLContext& ctx, RenderArgs2 args) -> void {
    const auto scene_rect = get_scene_rect(args.settings.view_config);

    build_layers(args.layout, args.settings.layers, args.selection, scene_rect);
    render_layers(ctx, args.layout, args.settings);
}

}  // namespace logicsim