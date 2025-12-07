#include "core/render/circuit/render_simulation_layer.h"

#include "core/allocated_size/std_vector.h"
#include "core/element/decoration/render_decoration_base.h"
#include "core/element/logicitem/render_logicitem_base.h"
#include "core/element/logicitem/render_logicitem_layer.h"
#include "core/format/container.h"
#include "core/geometry/rect.h"
#include "core/layout.h"
#include "core/render/circuit/render_connector.h"
#include "core/render/circuit/render_wire.h"
#include "core/render/context.h"
#include "core/spatial_simulation.h"
#include "core/vocabulary/drawable_element.h"

namespace logicsim {

auto SimulationLayers::format() const -> std::string {
    return fmt::format(
        "InteractiveLayers("
        "\n  items_below = {}"
        "\n  wires = {}"
        "\n  items_above = {}"
        "\n  decorations = {}"
        "\n)",

        items_below,  //
        wires,        //
        items_above,  //
        decorations   //
    );
}

auto SimulationLayers::allocated_size() const -> std::size_t {
    return get_allocated_size(items_below) +  //
           get_allocated_size(wires) +        //
           get_allocated_size(items_above) +  //
           get_allocated_size(decorations);
}

auto SimulationLayers::size() const -> std::size_t {
    return items_below.size() +  //
           wires.size() +        //
           items_above.size() +  //
           decorations.size();   //
}

auto SimulationLayers::empty() const -> bool {
    return this->size() == std::size_t {0};
}

auto build_simulation_layers(const Layout& layout,
                             rect_t scene_rect) -> SimulationLayers {
    auto layers = SimulationLayers {};

    for (const auto logicitem_id : logicitem_ids(layout)) {
        // visibility
        const auto bounding_rect = layout.logicitems().bounding_rect(logicitem_id);
        if (!is_colliding(bounding_rect, scene_rect)) {
            continue;
        }

        if (layout.logicitems().display_state(logicitem_id) == display_state_t::normal) {
            const auto type = layout.logicitems().type(logicitem_id);
            if (draw_logicitem_above(type)) {
                layers.items_above.push_back(logicitem_id);
            } else {
                layers.items_below.push_back(logicitem_id);
            }
        }
    }

    for (const auto decoration_id : decoration_ids(layout)) {
        // visibility
        const auto bounding_rect = layout.decorations().bounding_rect(decoration_id);
        if (!is_colliding(bounding_rect, scene_rect)) {
            continue;
        }

        if (layout.decorations().display_state(decoration_id) ==
            display_state_t::normal) {
            layers.decorations.push_back(decoration_id);
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
    ctx.bl_ctx.set_comp_op(BL_COMP_OP_SRC_COPY);

    draw_logicitems_base(ctx, spatial_simulation, layers.items_below);
    draw_wires(ctx, spatial_simulation, layers.wires);
    draw_logicitems_base(ctx, spatial_simulation, layers.items_above);

    draw_logicitems_connectors(ctx, spatial_simulation, layers.items_below);
    draw_logicitems_connectors(ctx, spatial_simulation, layers.items_above);
    draw_decorations_base(ctx, spatial_simulation.layout(), layers.decorations,
                          ElementDrawState::simulated);
};

}  // namespace logicsim
