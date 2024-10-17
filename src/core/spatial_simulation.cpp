#include "core/spatial_simulation.h"

#include "core/layout.h"
#include "core/line_tree_generation.h"
#include "core/schematic_generation.h"

#include <fmt/core.h>
#include <gsl/gsl>

namespace logicsim {

SpatialSimulation::SpatialSimulation() : SpatialSimulation {Layout {}, delay_t {1us}} {}

SpatialSimulation::SpatialSimulation(Layout &&layout, delay_t wire_delay_per_distance)
    : layout_ {std::move(layout)},
      line_trees_ {generate_line_trees(layout_)},
      wire_delay_per_distance_ {wire_delay_per_distance},
      simulation_ {generate_schematic(layout_, line_trees_, wire_delay_per_distance)} {}

auto SpatialSimulation::layout() const -> const Layout & {
    return layout_;
}

auto SpatialSimulation::schematic() const -> const Schematic & {
    return simulation_.schematic();
}

auto SpatialSimulation::simulation() const -> const Simulation & {
    return simulation_;
}

auto SpatialSimulation::simulation() -> Simulation & {
    return simulation_;
}

auto SpatialSimulation::line_tree(wire_id_t wire_id) const -> const LineTree & {
    return line_trees_.at(wire_id.value);
}

auto SpatialSimulation::wire_delay_per_distance() const -> delay_t {
    return wire_delay_per_distance_;
}

auto SpatialSimulation::format() const -> std::string {
    return fmt::format("SpatialSimulation(\n  {},\n  {}\n  {}\n)", layout_,
                       simulation_.schematic(), simulation_);
}

//
// Free Functions
//

auto to_element_id(const SpatialSimulation &spatial_simulation,
                   logicitem_id_t logicitem_id) -> element_id_t {
    return to_element_id(spatial_simulation.layout(), logicitem_id);
}

auto to_element_id(const SpatialSimulation &spatial_simulation,
                   wire_id_t wire_id) -> element_id_t {
    return to_element_id(spatial_simulation.layout(), wire_id);
}

auto to_logicitem_id(const SpatialSimulation &spatial_simulation,
                     element_id_t element_id) -> logicitem_id_t {
    return to_logicitem_id(spatial_simulation.layout(), element_id);
}

auto to_wire_id(const SpatialSimulation &spatial_simulation,
                element_id_t element_id) -> wire_id_t {
    return to_wire_id(spatial_simulation.layout(), element_id);
}

}  // namespace logicsim
