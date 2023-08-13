#include "render_circuit.h"

#include "collision.h"
#include "editable_circuit/selection.h"
#include "layout.h"
#include "layout_calculation.h"
#include "timer.h"

#include <numbers>

namespace logicsim {

//
// Element Draw State
//

template <>
auto format(ElementDrawState state) -> std::string {
    switch (state) {
        using enum ElementDrawState;

        case normal:
            return "normal";
        case selected:
            return "selected";
        case uninserted:
            return "uninserted";
        case simulated:
            return "simulated";
    }

    throw_exception("cannot convert ElementDrawState to string");
}

//
// Logic Items
//

auto render_above(ElementType type [[maybe_unused]]) -> bool {
    return type == ElementType::button;
}

auto draw_logic_item_rect(BLContext& ctx, rect_fine_t rect, layout::ConstElement element,
                          ElementDrawState state, const RenderSettings& settings)
    -> void {
    const auto final_rect = rect + point_fine_t {element.position()};

    draw_rect(ctx, final_rect,
              RectAttributes {
                  .draw_type = DrawType::fill_and_stroke,
                  .fill_color = defaults::body_color_normal,
                  .stroke_color = defaults::body_stroke_color,
              },
              settings);
}

auto draw_standard_element(BLContext& ctx, layout::ConstElement element,
                           ElementDrawState state, const RenderSettings& settings)
    -> void {
    const auto element_height = std::max(element.input_count(), element.output_count());
    const auto padding = defaults::logic_item_body_overdraw;
    const auto rect = rect_fine_t {point_fine_t {0., -padding},
                                   point_fine_t {2., element_height - 1 + padding}};

    draw_logic_item_rect(ctx, rect, element, state, settings);
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

            // case buffer_element:
            //     return draw_buffer(ctx, element, selected, settings);

        case and_element:
        case or_element:
        case xor_element:
            return draw_standard_element(ctx, element, state, settings);

            // case button:
            //     return draw_button(ctx, element, selected, settings);

            // case clock_generator:
            //     return draw_clock_generator(ctx, element, selected, settings);
            // case flipflop_jk:
            //     return draw_flipflop_jk(ctx, element, selected, settings);
            // case shift_register:
            //     return draw_shift_register(ctx, element, selected, settings);
            // case latch_d:
            //     return draw_latch_d(ctx, element, selected, settings);
            // case flipflop_d:
            //     return draw_flipflop_d(ctx, element, selected, settings);
            // case flipflop_ms_d:
            //     return draw_flipflop_ms_d(ctx, element, selected, settings);

            // case sub_circuit:
            //     return draw_standard_element(ctx, element, selected, settings);

        default:  // TODO !!! remove this
            return;
    }
    throw_exception("not supported");
}

auto draw_logic_item_connectors(BLContext& ctx, layout::ConstElement element,
                                ElementDrawState state, const RenderSettings& settings)
    -> void {}

auto draw_logic_items(BLContext& ctx, const Layout& layout,
                      std::span<const element_id_t> elements,
                      const RenderSettings& settings) -> void {
    for (const auto element_id : elements) {
        draw_logic_item(ctx, layout.element(element_id), false, settings);
    }
}

//
// Wires
//

auto draw_wires(BLContext& ctx, const Layout& layout,
                std::span<const element_id_t> elements, const RenderSettings& settings)
    -> void {
    for (const auto element_id : elements) {
        draw_segment_tree(ctx, layout.element(element_id), settings);
    }
}

auto draw_wires(BLContext& ctx, std::span<const segment_info_t> segments,
                const RenderSettings& settings) -> void {
    for (const auto& segment : segments) {
        draw_line_segment(ctx, segment.line, false, settings);

        if (is_cross_point(segment.p0_type)) {
            draw_line_cross_point(ctx, segment.line.p0, false, settings);
        }
        if (is_cross_point(segment.p1_type)) {
            draw_line_cross_point(ctx, segment.line.p1, false, settings);
        }
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
            return defaults::overlay_selected;
        }
        case shadow_t::valid: {
            return defaults::overlay_valid;
        }
        case shadow_t::colliding: {
            return defaults::overlay_colliding;
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

auto draw_wire_shadows(BLContext& ctx, std::span<const ordered_line_t> lines,
                       shadow_t shadow_type, const RenderSettings& settings) -> void {
    const auto color = shadow_color(shadow_type);
    ctx.setFillStyle(BLRgba32(color.value));

    for (auto line : lines) {
        const auto selection_rect = element_selection_rect_rounded(line);
        draw_round_rect(ctx, selection_rect, {.draw_type = DrawType::fill}, settings);
    }
}

//
// Layout Rendering
//

auto render_inserted(BLContext& ctx, const Layout& layout,
                     const RenderSettings& settings) {
    const LayersCache& layers = settings.layers;

    ctx.setCompOp(BL_COMP_OP_SRC_COPY);

    draw_logic_items(ctx, layout, layers.normal_below, settings);
    draw_wires(ctx, layout, layers.normal_wires, settings);
    draw_logic_items(ctx, layout, layers.normal_above, settings);
}

auto render_uninserted(BLContext& ctx, const Layout& layout,
                       const RenderSettings& settings) {
    const LayersCache& layers = settings.layers;

    if (settings.layer_surface_uninserted.enabled) {
        ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    } else {
        ctx.setCompOp(BL_COMP_OP_SRC_OVER);
    }

    draw_logic_items(ctx, layout, layers.uninserted_below, settings);
    draw_wires(ctx, layers.uninserted_wires, settings);
    draw_logic_items(ctx, layout, layers.uninserted_above, settings);
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
            const auto display_state = element.display_state();

            const auto selected = (selection != nullptr)
                                      ? selection->is_selected(element.element_id())
                                      : false;
            bool has_overlay = false;

            if (is_inserted(display_state)) {
                if (!render_above(element_type)) {
                    layers.normal_below.push_back(element);
                } else {
                    layers.normal_above.push_back(element);
                }

                if (display_state == display_state_t::valid) {
                    layers.valid_logic_items.push_back(element);
                    has_overlay = true;
                } else if (selected) {
                    layers.selected_logic_items.push_back(element);
                    has_overlay = true;
                }
            } else {
                if (!render_above(element_type)) {
                    layers.uninserted_below.push_back(element);
                } else {
                    layers.uninserted_above.push_back(element);
                }
                update_uninserted_rect(layers, bounding_rect);

                if (display_state == display_state_t::colliding) {
                    layers.colliding_logic_items.push_back(element);
                    has_overlay = true;
                } else if (selected) {
                    layers.selected_logic_items.push_back(element);
                    has_overlay = true;
                }
            }
            if (has_overlay) {
                update_overlay_rect(layers, bounding_rect);
            }
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
                        layers.uninserted_wires.push_back(info);
                        update_uninserted_rect(layers, info.line);

                        if (display_state == display_state_t::colliding) {
                            layers.colliding_wires.push_back(info.line);
                        } else if (display_state == display_state_t::temporary) {
                            layers.temporary_wires.push_back(info.line);
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

auto render_circuit_2(BLContext& ctx, render_args_t args) -> void {
    const auto scene_rect = get_scene_rect(args.settings.view_config);

    build_layers(args.layout, args.settings.layers, args.selection, scene_rect);
    render_layers(ctx, args.layout, args.settings);
}

}  // namespace logicsim