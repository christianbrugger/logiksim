#include "render/circuit/render_simulation_layer.h"

#include "allocated_size/std_vector.h"
#include "format/container.h"
#include "geometry/rect.h"
#include "layout.h"
#include "element/logicitem/render_logicitem_base.h"
#include "element/logicitem/render_logicitem_layer.h"
#include "render/circuit/render_connector.h"
#include "render/circuit/render_wire.h"
#include "render/context.h"
#include "spatial_simulation.h"

namespace logicsim {

auto SimulationLayers::format() const -> std::string {
    return fmt::format(
        "InteractiveLayers("
        "\n  items_below = {}"
        "\n  wires = {}"
        "\n  items_above = {}"
        "\n)",

        items_below,  //
        wires,        //
        items_above   //
    );
}

auto SimulationLayers::allocated_size() const -> std::size_t {
    return get_allocated_size(items_below) +  //
           get_allocated_size(wires) +        //
           get_allocated_size(items_above);
}

auto SimulationLayers::size() const -> std::size_t {
    return items_below.size() +  //
           wires.size() +        //
           items_above.size();   //
}

auto SimulationLayers::empty() const -> bool {
    return this->size() == std::size_t {0};
}

auto build_simulation_layers(const Layout& layout,
                             rect_t scene_rect) -> SimulationLayers {
    auto layers = SimulationLayers {};

    for (const auto logicitem_id : logicitem_ids(layout)) {
        // visibility
        const auto bounding_rect = layout.logic_items().bounding_rect(logicitem_id);
        if (!is_colliding(bounding_rect, scene_rect)) {
            continue;
        }

        if (layout.logic_items().display_state(logicitem_id) == display_state_t::normal) {
            const auto type = layout.logic_items().type(logicitem_id);
            if (draw_logic_item_above(type)) {
                layers.items_above.push_back(logicitem_id);
            } else {
                layers.items_below.push_back(logicitem_id);
            }
        }
    }

    for (const auto wire_id : inserted_wire_ids(layout)) {
        // visibility
        const auto bounding_rect = layout.wires().bounding_rect(wire_id);
        if (!is_colliding(bounding_rect, scene_rect)) {
            continue;
        }

        layers.wires.push_back(wire_id);
    }

    return layers;
}

auto render_simulation_layers(Context& ctx, const SpatialSimulation& spatial_simulation,
                              const SimulationLayers& layers) -> void {
    ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_COPY);

    draw_logic_items_base(ctx, spatial_simulation, layers.items_below);
    draw_wires(ctx, spatial_simulation, layers.wires);
    draw_logic_items_base(ctx, spatial_simulation, layers.items_above);

    draw_logic_items_connectors(ctx, spatial_simulation, layers.items_below);
    draw_logic_items_connectors(ctx, spatial_simulation, layers.items_above);
};

}  // namespace logicsim
