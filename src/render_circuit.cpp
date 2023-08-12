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

auto build_layers(const Layout& layout, const LayersCache& layers,
                  const Selection* selection, rect_t scene_rect) -> void {
    layers.clear();

    for (const auto element : layout.elements()) {
        // visibility check
        if (!is_colliding(element.bounding_rect(), scene_rect)) {
            continue;
        }
        const auto element_type = element.element_type();
        if (is_logic_item(element_type)) {
            const auto display_state = element.display_state();

            const auto selected = (selection != nullptr)
                                      ? selection->is_selected(element.element_id())
                                      : false;

            if (is_inserted(display_state)) {
                if (!render_above(element_type)) {
                    layers.normal_below.push_back(element);
                } else {
                    layers.normal_above.push_back(element);
                }

                if (display_state == display_state_t::valid) {
                    layers.valid_logic_items.push_back(element);
                } else if (selected) {
                    layers.selected_logic_items.push_back(element);
                }
            } else {
                if (!render_above(element_type)) {
                    layers.uninserted_below.push_back(element);
                } else {
                    layers.uninserted_above.push_back(element);
                }

                if (display_state == display_state_t::colliding) {
                    layers.colliding_logic_items.push_back(element);
                } else if (selected) {
                    layers.selected_logic_items.push_back(element);
                }
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
                // uninserted segment trees can contain a lot of segments,
                // therefore do a fine grained collision check
                for (const auto& info : element.segment_tree().segment_infos()) {
                    if (is_colliding(info.line, scene_rect)) {
                        layers.uninserted_wires.push_back(info);

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

    draw_rect(ctx, selection_rect, {.draw_type = DrawType::fill}, settings);
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

        /*
        auto rect0 = to_context(selection_rect.p0, settings.view_config);
        auto rect1 = to_context(selection_rect.p1, settings.view_config);

        BLPoint p0;
        BLPoint p1;
        double r;

        if (is_horizontal(line)) {
            p0.x = rect0.x + 0.5;
            p1.x = rect1.x + 0.5;

            p0.y = p1.y = (rect0.y + rect1.y + 1) / 2;
            r = (rect1.y - rect0.y + 1) / 2;
        } else {
            p0.y = rect0.y + 0.5;
            p1.y = rect1.y + 0.5;  // 0.5;

            p0.x = p1.x = (rect0.x + rect1.x + 1) / 2;
            r = (rect1.x - rect0.x + 1) / 2;
        }

        ctx.fillCircle(BLCircle(p0.x, p0.y, r));
        ctx.fillCircle(BLCircle(p1.x, p1.y, r));
        */
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

auto render_overlay_impl(BLContext& ctx, const Layout& layout,
                         const RenderSettings& settings) -> void {
    const LayersCache& cache = settings.layers;

    // selected & temporary
    draw_logic_item_shadows(ctx, layout, cache.selected_logic_items, shadow_t::selected,
                            settings);
    draw_wire_shadows(ctx, cache.selected_wires, shadow_t::selected, settings);
    draw_wire_shadows(ctx, cache.temporary_wires, shadow_t::selected, settings);

    // valid
    draw_logic_item_shadows(ctx, layout, cache.valid_logic_items, shadow_t::valid,
                            settings);
    draw_wire_shadows(ctx, cache.valid_wires, shadow_t::valid, settings);

    // colliding
    draw_logic_item_shadows(ctx, layout, cache.colliding_logic_items, shadow_t::colliding,
                            settings);
    draw_wire_shadows(ctx, cache.colliding_wires, shadow_t::colliding, settings);
}

auto render_overlay(BLContext& target_ctx, const Layout& layout,
                    const RenderSettings& settings) -> void {
    if (settings.separate_layer) {
        Layer& layer = settings.layer_overlay;
        layer.initialize(settings.view_config, context_info(settings));

        layer.ctx.clearAll();
        layer.ctx.setCompOp(BL_COMP_OP_SRC_COPY);
        render_overlay_impl(layer.ctx, layout, settings);

        layer.ctx.flush(BL_CONTEXT_FLUSH_SYNC);
        target_ctx.blitImage(BLPointI {0, 0}, layer.image);
    } else {
        render_overlay_impl(target_ctx, layout, settings);
    }
}

auto render_layers(BLContext& ctx, const Layout& layout, const RenderSettings& settings)
    -> void {
    const LayersCache& layers = settings.layers;

    // TODO draw with alpha here anything ???
    // TODO draw line inverters / connectors above wires
    // TODO draw uninserted wires in shadow

    draw_logic_items(ctx, layout, layers.normal_below, settings);
    draw_wires(ctx, layout, layers.normal_wires, settings);
    draw_logic_items(ctx, layout, layers.normal_above, settings);

    draw_logic_items(ctx, layout, layers.uninserted_below, settings);
    draw_wires(ctx, layers.uninserted_wires, settings);
    draw_logic_items(ctx, layout, layers.uninserted_above, settings);

    render_overlay(ctx, layout, settings);
}

auto render_circuit_2(BLContext& ctx, render_args_t args) -> void {
    const auto scene_rect = get_scene_rect(args.settings.view_config);

    build_layers(args.layout, args.settings.layers, args.selection, scene_rect);
    render_layers(ctx, args.layout, args.settings);

    // print(args.settings.layers);
}
}  // namespace logicsim