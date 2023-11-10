#ifndef LOGICSIM_SPATIAL_SIMULATION_H
#define LOGICSIM_SPATIAL_SIMULATION_H

#include "layout.h"
#include "line_tree.h"
#include "simulation.h"
#include "vocabulary/simulation_setting.h"

#include <optional>
#include <vector>

namespace logicsim {

/**
 * @brief: Simulation holding also the spatial information for wires and logic items.
 *
 * Also holds information to convert between element and logicitem and wire ids.
 *
 * Class-invariants:
 *     + The layout does never change.
 *     + The simulation matches the given layout.
 */
class SpatialSimulation {
   public:
    [[nodiscard]] explicit SpatialSimulation(Layout &&layout,
                                             delay_t wire_delay_per_distance);

    [[nodiscard]] auto layout() const -> const Layout &;
    [[nodiscard]] auto schematic() const -> const Schematic &;
    [[nodiscard]] auto simulation() const -> const Simulation &;
    [[nodiscard]] auto simulation() -> Simulation &;

    [[nodiscard]] auto line_tree(wire_id_t wire_id) const
        -> const std::optional<LineTree> &;

    [[nodiscard]] auto wire_delay_per_distance() const -> delay_t;

   private:
    Layout layout_;
    std::vector<std::optional<LineTree>> line_trees_;

    delay_t wire_delay_per_distance_;
    Simulation simulation_;
};

[[nodiscard]] auto to_element_id(const SpatialSimulation &spatial_simulation,
                                 logicitem_id_t logicitem_id) -> element_id_t;
[[nodiscard]] auto to_element_id(const SpatialSimulation &spatial_simulation,
                                 wire_id_t wire_id) -> element_id_t;
[[nodiscard]] auto to_logicitem_id(const SpatialSimulation &spatial_simulation,
                                   element_id_t element_id) -> logicitem_id_t;
[[nodiscard]] auto to_wire_id(const SpatialSimulation &spatial_simulation,
                              element_id_t element_id) -> wire_id_t;

}  // namespace logicsim

#endif
