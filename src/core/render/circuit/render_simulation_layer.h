#ifndef LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_SIMULATION_LAYER_H
#define LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_SIMULATION_LAYER_H

#include "core/format/struct.h"
#include "core/vocabulary/decoration_id.h"
#include "core/vocabulary/logicitem_id.h"
#include "core/vocabulary/wire_id.h"

#include <concepts>
#include <vector>

namespace logicsim {

struct rect_t;
class Layout;
class SpatialSimulation;
struct Context;

struct SimulationLayers {
    // inserted
    std::vector<logicitem_id_t> items_below;
    std::vector<wire_id_t> wires;
    std::vector<logicitem_id_t> items_above;
    std::vector<decoration_id_t> decorations;

   public:
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const SimulationLayers&) const -> bool = default;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto empty() const -> bool;
};

static_assert(std::regular<SimulationLayers>);

[[nodiscard]] auto build_simulation_layers(const Layout& layout,
                                           rect_t scene_rect) -> SimulationLayers;

auto render_simulation_layers(Context& ctx, const SpatialSimulation& spatial_simulation,
                              const SimulationLayers& layers) -> void;

}  // namespace logicsim

#endif
