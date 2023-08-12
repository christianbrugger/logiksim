#include "render_circuit.h"

#include "collision.h"
#include "editable_circuit/selection.h"
#include "layout_calculations.h"
#include "timer.h"

#include <numbers>

namespace logicsim {

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

auto render_above(ElementType type [[maybe_unused]]) -> bool {
    // TODO only buttons
    return true;  // type == ElementType::button;
}

auto element_shadow_rounding(ElementType type [[maybe_unused]]) -> double {
    return type == ElementType::button ? 0. : 0.3;
}

auto draw_logic_items(BLContext& ctx, const Layout& layout,
                      std::span<const element_id_t> elements,
                      const RenderSettings& settings) -> void {
    for (const auto element_id : elements) {
        draw_logic_item(ctx, layout.element(element_id), false, settings);
    }
}

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

auto shadow_color(shadow_t shadow_type) -> color_t {
    switch (shadow_type) {
        case shadow_t::selected: {
            // BLRgba32(0, 128, 255, 96);
            return color_t {0x600080FF};
        }
        case shadow_t::valid: {
            // BLRgba32(0, 192, 0, 96);
            return color_t {0x6000C000};
        }
        case shadow_t::colliding: {
            // BLRgba32(255, 0, 0, 96);
            return color_t {0x60FF0000};
        }
    };

    throw_exception("unknown shadow type");
}

auto draw_logic_item_shadow(BLContext& ctx, layout::ConstElement element,
                            shadow_t shadow_type, const RenderSettings& settings)
    -> void {
    const auto data = to_layout_calculation_data(element.layout(), element);
    const auto selection_rect = element_selection_rect(data);

    const auto color = shadow_color(shadow_type);
    ctx.setFillStyle(BLRgba32(color.value));

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

/*
auto draw_wire_shadows(BLContext& ctx, const Layout& layout,
                       std::span<const element_id_t> elements, shadow_t shadow_type,
                       const RenderSettings& settings) -> void {
    const auto color = shadow_color(shadow_type);
    ctx.setFillStyle(BLRgba32(color.value));

    for (const auto element_id : elements) {
        for (const auto& info : layout.segment_tree(element_id).segment_infos()) {
            const auto selection_rect = element_selection_rect(info.line);
            draw_rect(ctx, selection_rect, {.draw_type = DrawType::fill}, settings);
        }
    }
}
*/

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

template <typename Func>
auto render_to_layer(BLContext& target_ctx, LayerSurface& layer, BLRectI dirty_rect,
                     const RenderSettings& settings, Func render_func) -> void {
    target_ctx.save();

    if (layer.enabled) {
        layer.initialize(settings.view_config, context_info(settings));
        layer.ctx.clearRect(dirty_rect);

        {
            layer.ctx.save();
            render_func(layer.ctx);
            layer.ctx.restore();
        }

        layer.ctx.flush(BL_CONTEXT_FLUSH_SYNC);
        target_ctx.setCompOp(BL_COMP_OP_SRC_OVER);
        target_ctx.blitImage(dirty_rect, layer.image, dirty_rect);
    } else {
        render_func(target_ctx);
    }

    target_ctx.restore();
}

auto get_dirty_rect(rect_t bounding_rect, const ViewConfig& view_config) -> BLRectI {
    const auto clamp_x = [&](double x_) {
        return std::clamp(x_, 0., view_config.width() * 1.0);
    };
    const auto clamp_y = [&](double y_) {
        return std::clamp(y_, 0., view_config.height() * 1.0);
    };

    const auto p0 = to_context(bounding_rect.p0, view_config);
    const auto p1 = to_context(bounding_rect.p1, view_config);

    const auto padding = view_config.pixel_scale() * 0.5 + 2;

    const auto x0 = clamp_x(std::trunc(p0.x - padding));
    const auto y0 = clamp_y(std::trunc(p0.y - padding));

    const auto x1 = clamp_x(std::ceil(p1.x + padding + 1));
    const auto y1 = clamp_y(std::ceil(p1.y + padding + 1));

    return BLRectI {
        gsl::narrow<int>(x0),
        gsl::narrow<int>(y0),
        gsl::narrow<int>(x1 - x0),
        gsl::narrow<int>(y1 - y0),
    };
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

        // ctx.setFillStyle(BLRgba32 {0xFF000000});
        // ctx.fillRect(rect);
        // print(rect.x, rect.y, rect.h, rect.w);

        render_to_layer(ctx, settings.layer_surface_uninserted, rect, settings,
                        [&](BLContext& layer_ctx) {
                            render_uninserted(layer_ctx, layout, settings);
                        });
    }

    if (settings.layers.overlay_bounding_rect.has_value()) {
        const auto rect = get_dirty_rect(settings.layers.overlay_bounding_rect.value(),
                                         settings.view_config);

        // ctx.setFillStyle(BLRgba32 {0xFF000000});
        // ctx.fillRect(rect);
        // print(rect.x, rect.y, rect.h, rect.w);

        render_to_layer(
            ctx, settings.layer_surface_overlay, rect, settings,
            [&](BLContext& layer_ctx) { render_overlay(layer_ctx, layout, settings); });
    }
}

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

auto render_circuit_2(BLContext& ctx, render_args_t args) -> void {
    const auto scene_rect = get_scene_rect(args.settings.view_config);

    build_layers(args.layout, args.settings.layers, args.selection, scene_rect);
    render_layers(ctx, args.layout, args.settings);

    // print(args.settings.layers);
}
}  // namespace logicsim