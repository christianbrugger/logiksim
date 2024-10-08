#include "render/circuit/render_interactive_layer.h"

#include "allocated_size/std_vector.h"
#include "format/container.h"
#include "format/std_type.h"
#include "geometry/rect.h"
#include "geometry/scene.h"
#include "layout.h"
#include "element/logicitem/render_logicitem_base.h"
#include "element/logicitem/render_logicitem_layer.h"
#include "render/circuit/render_connector.h"
#include "render/circuit/render_overlay.h"
#include "render/circuit/render_wire.h"
#include "render/context.h"
#include "render/image_surface.h"
#include "selection.h"

namespace logicsim {

auto to_element_draw_state(const Layout& layout, logicitem_id_t logicitem_id,
                           const Selection* selection) -> ElementDrawState {
    const auto is_selected = [&]() {
        return (selection != nullptr) ? selection->is_selected(logicitem_id) : false;
    };

    const auto display_state = layout.logicitems().display_state(logicitem_id);

    if (is_inserted(display_state)) {
        if (display_state == display_state_t::valid) {
            return ElementDrawState::valid;
        }
        if (is_selected()) {
            return ElementDrawState::normal_selected;
        }
        return ElementDrawState::normal;
    }

    if (display_state == display_state_t::colliding) {
        return ElementDrawState::colliding;
    }
    if (is_selected()) {
        return ElementDrawState::temporary_selected;
    }
    throw std::runtime_error("temporary items have no cannot be drawn");
}

auto InteractiveLayers::format() const -> std::string {
    return fmt::format(
        "InteractiveLayers("
        "\n  normal_below = {}"
        "\n  normal_wires = {}"
        "\n  normal_above = {}"
        "\n"
        "\n  uninserted_below = {}"
        "\n  uninserted_above = {}"
        "\n"
        "\n  selected_logicitems = {}"
        "\n  selected_wires = {}"
        "\n  temporary_wires = {}"
        "\n  valid_logicitems = {}"
        "\n  valid_wires = {}"
        "\n  colliding_logicitems = {}"
        "\n  colliding_wires = {}"
        "\n"
        "\n  uninserted_bounding_rect = {}"
        "\n  overlay_bounding_rect = {}"
        "\n)",

        normal_below,  //
        normal_wires,  //
        normal_above,  //

        uninserted_below,  //
        uninserted_above,  //

        selected_logicitems,   //
        selected_wires,         //
        temporary_wires,        //
        valid_logicitems,      //
        valid_wires,            //
        colliding_logicitems,  //
        colliding_wires,        //

        uninserted_bounding_rect,  //
        overlay_bounding_rect      //
    );
}

auto InteractiveLayers::allocated_size() const -> std::size_t {
    return get_allocated_size(normal_below) +           //
           get_allocated_size(normal_wires) +           //
           get_allocated_size(normal_above) +           //
                                                        //
           get_allocated_size(uninserted_below) +       //
           get_allocated_size(uninserted_above) +       //
                                                        //
           get_allocated_size(selected_logicitems) +   //
           get_allocated_size(selected_wires) +         //
           get_allocated_size(temporary_wires) +        //
           get_allocated_size(valid_logicitems) +      //
           get_allocated_size(valid_wires) +            //
           get_allocated_size(colliding_logicitems) +  //
           get_allocated_size(colliding_wires);         //
}

auto InteractiveLayers::size() const -> std::size_t {
    return normal_below.size() +           //
           normal_wires.size() +           //
           normal_above.size() +           //
                                           //
           uninserted_below.size() +       //
           uninserted_above.size() +       //
                                           //
           selected_logicitems.size() +   //
           selected_wires.size() +         //
           temporary_wires.size() +        //
           valid_logicitems.size() +      //
           valid_wires.size() +            //
           colliding_logicitems.size() +  //
           colliding_wires.size();         //
}

auto InteractiveLayers::empty() const -> bool {
    return this->size() == std::size_t {0};
}

auto InteractiveLayers::has_inserted() const -> bool {
    return !normal_below.empty() ||  //
           !normal_wires.empty() ||  //
           !normal_above.empty();
}

auto InteractiveLayers::has_uninserted() const -> bool {
    return !uninserted_below.empty() ||  //
           !temporary_wires.empty() ||   //
           !colliding_wires.empty() ||   //
           !uninserted_above.empty();
}

auto InteractiveLayers::has_overlay() const -> bool {
    return !selected_logicitems.empty() ||   //
           !selected_wires.empty() ||         //
           !temporary_wires.empty() ||        //
           !valid_logicitems.empty() ||      //
           !valid_wires.empty() ||            //
           !colliding_logicitems.empty() ||  //
           !colliding_wires.empty();
}

auto InteractiveLayers::calculate_overlay_bounding_rect() -> void {
    const auto update = [this](ordered_line_t line) { update_overlay_rect(*this, line); };
    const auto update_info = [this](segment_info_t info) {
        update_overlay_rect(*this, info.line);
    };

    std::ranges::for_each(selected_wires, update);
    std::ranges::for_each(temporary_wires, update_info);
    std::ranges::for_each(valid_wires, update);
    std::ranges::for_each(colliding_wires, update_info);
}

auto update_bounding_rect(std::optional<rect_t>& target, rect_t new_rect) -> void {
    if (!target) {
        target = new_rect;
    } else {
        *target = enclosing_rect(*target, new_rect);
    }
}

auto update_bounding_rect(std::optional<rect_t>& target,
                          ordered_line_t new_line) -> void {
    if (!target) {
        target = rect_t {new_line.p0, new_line.p1};
    } else {
        *target = enclosing_rect(*target, new_line);
    }
}

auto update_uninserted_rect(InteractiveLayers& layers, rect_t bounding_rect) -> void {
    update_bounding_rect(layers.uninserted_bounding_rect, bounding_rect);
}

auto update_uninserted_rect(InteractiveLayers& layers, ordered_line_t line) -> void {
    update_bounding_rect(layers.uninserted_bounding_rect, line);
}

auto update_overlay_rect(InteractiveLayers& layers, rect_t bounding_rect) -> void {
    update_bounding_rect(layers.overlay_bounding_rect, bounding_rect);
}

auto update_overlay_rect(InteractiveLayers& layers, ordered_line_t line) -> void {
    update_bounding_rect(layers.overlay_bounding_rect, line);
}

//
// Build
//

namespace {

auto add_valid_wire_parts(const Layout& layout, wire_id_t wire_id,
                          std::vector<ordered_line_t>& output) -> bool {
    auto found = false;

    const auto& tree = layout.wires().segment_tree(wire_id);

    for (const segment_index_t& index : tree.indices()) {
        for (const ordered_line_t& valid_line : all_valid_lines(tree, index)) {
            output.push_back(valid_line);
            found = true;
        }
    }

    return found;
}

auto add_selected_wire_parts(const Layout& layout, wire_id_t wire_id,
                             const Selection& selection,
                             std::vector<ordered_line_t>& output) -> void {
    const auto& tree = layout.wires().segment_tree(wire_id);

    for (const auto segment : tree.indices(wire_id)) {
        const auto& parts = selection.selected_segments(segment);

        if (parts.empty()) {
            continue;
        }

        const auto full_line = tree.line(segment.segment_index);

        for (const auto part : parts) {
            output.push_back(to_line(full_line, part));
        }
    }
}

auto insert_logicitem(InteractiveLayers& layers, const Layout& layout,
                       logicitem_id_t logicitem_id, rect_t bounding_rect,
                       ElementDrawState state) -> void {
    const auto logicitem_type = layout.logicitems().type(logicitem_id);

    if (is_inserted(state)) {
        if (draw_logicitem_above(logicitem_type)) {
            layers.normal_above.push_back({logicitem_id, state});
        } else {
            layers.normal_below.push_back({logicitem_id, state});
        }
    } else {
        update_uninserted_rect(layers, bounding_rect);

        if (draw_logicitem_above(logicitem_type)) {
            layers.uninserted_above.push_back({logicitem_id, state});
        } else {
            layers.uninserted_below.push_back({logicitem_id, state});
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
            layers.selected_logicitems.push_back(logicitem_id);
            break;
        case valid:
            layers.valid_logicitems.push_back(logicitem_id);
            break;
        case colliding:
            layers.colliding_logicitems.push_back(logicitem_id);
            break;
    }
}

}  // namespace

auto build_interactive_layers(const Layout& layout, const Selection* selection,
                              rect_t scene_rect) -> InteractiveLayers {
    auto layers = InteractiveLayers {};

    for (const auto logicitem_id : logicitem_ids(layout)) {
        // visibility
        const auto bounding_rect = layout.logicitems().bounding_rect(logicitem_id);
        if (!is_colliding(bounding_rect, scene_rect)) {
            continue;
        }

        const auto state = to_element_draw_state(layout, logicitem_id, selection);
        insert_logicitem(layers, layout, logicitem_id, bounding_rect, state);
    }

    for (const auto wire_id : inserted_wire_ids(layout)) {
        // visibility
        const auto bounding_rect = layout.wires().bounding_rect(wire_id);
        if (!is_colliding(bounding_rect, scene_rect)) {
            continue;
        }

        layers.normal_wires.push_back(wire_id);

        // TODO add: tree.has_valid_parts()
        const auto found_valid =
            add_valid_wire_parts(layout, wire_id, layers.valid_wires);

        if (!found_valid && selection != nullptr) {
            add_selected_wire_parts(layout, wire_id, *selection, layers.selected_wires);
        }
    }
    // fine grained check, as uninserted trees can contain a lot of segments
    for (const auto& info : layout.wires().segment_tree(temporary_wire_id)) {
        if (is_colliding(info.line, scene_rect)) {
            update_uninserted_rect(layers, info.line);
            layers.temporary_wires.push_back(info);
        }
    }
    for (const auto& info : layout.wires().segment_tree(colliding_wire_id)) {
        if (is_colliding(info.line, scene_rect)) {
            update_uninserted_rect(layers, info.line);
            layers.colliding_wires.push_back(info);
        }
    }

    layers.calculate_overlay_bounding_rect();

    return layers;
}

//
// Render
//

auto render_inserted(Context& ctx, const Layout& layout,
                     const InteractiveLayers& layers) -> void {
    ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_COPY);

    draw_logicitems_base(ctx, layout, layers.normal_below);
    draw_wires(ctx, layout, layers.normal_wires, ElementDrawState::normal);
    draw_logicitems_base(ctx, layout, layers.normal_above);

    draw_logicitems_connectors(ctx, layout, layers.normal_below);
    draw_logicitems_connectors(ctx, layout, layers.normal_above);
}

auto render_uninserted(Context& ctx, const Layout& layout,
                       const InteractiveLayers& layers, bool layer_enabled) -> void {
    if (layer_enabled) {
        ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    } else {
        ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_OVER);
    }

    draw_logicitems_base(ctx, layout, layers.uninserted_below);
    draw_wires(ctx, layers.temporary_wires, ElementDrawState::temporary_selected);
    draw_wires(ctx, layers.colliding_wires, ElementDrawState::colliding);
    draw_logicitems_base(ctx, layout, layers.uninserted_above);

    draw_logicitems_connectors(ctx, layout, layers.uninserted_below);
    draw_logicitems_connectors(ctx, layout, layers.uninserted_above);
}

auto render_overlay(Context& ctx, const Layout& layout, const InteractiveLayers& layers,
                    bool layer_enabled) -> void {
    if (layer_enabled) {
        ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    } else {
        ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_OVER);
    }

    // selected & temporary
    draw_logicitem_shadows(ctx, layout, layers.selected_logicitems, shadow_t::selected);
    draw_wire_shadows(ctx, layers.selected_wires, shadow_t::selected);
    draw_wire_shadows(ctx, layers.temporary_wires, shadow_t::selected);

    // valid
    draw_logicitem_shadows(ctx, layout, layers.valid_logicitems, shadow_t::valid);
    draw_wire_shadows(ctx, layers.valid_wires, shadow_t::valid);

    // colliding
    draw_logicitem_shadows(ctx, layout, layers.colliding_logicitems,
                            shadow_t::colliding);
    draw_wire_shadows(ctx, layers.colliding_wires, shadow_t::colliding);
}

auto render_interactive_layers(Context& ctx, const Layout& layout,
                               const InteractiveLayers& layers,
                               ImageSurface& surface) -> void {
    if (layers.has_inserted()) {
        render_inserted(ctx, layout, layers);
    }

    const auto layer_enabled = true;  // TODO: remove

    if (layers.uninserted_bounding_rect.has_value()) {
        const auto rect =
            get_dirty_rect(layers.uninserted_bounding_rect.value(), ctx.view_config());

        render_layer(ctx, surface, rect, [&](Context& layer_ctx) {
            render_uninserted(layer_ctx, layout, layers, layer_enabled);
        });
    }

    if (layers.overlay_bounding_rect.has_value()) {
        const auto rect =
            get_dirty_rect(layers.overlay_bounding_rect.value(), ctx.view_config());

        render_layer(ctx, surface, rect, [&](Context& layer_ctx) {
            render_overlay(layer_ctx, layout, layers, layer_enabled);
        });
    }
}

}  // namespace logicsim
