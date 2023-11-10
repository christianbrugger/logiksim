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
 *     + The simulation is generated from the given layout.
 *     + The line trees are equivalent to the segment trees in the layout
 *     + The segment trees in the layout form contiguous trees.
 *     + Segment trees in the layout have all cross-points set.
 *     + Schematic is created with wire_delay_per_distance_ and it never changes.
 */
class SpatialSimulation {
   public:
    /**
     * @brief: Create a new spatial simulation.
     *
     * Pre-condition: All inserted segment-trees are expected to form contiguous trees.
     * Pre-condition: All inserted segment-trees have all cross-points set.
     *
     * Note this can be achieved through methods in `tree_normalization.h`.
     */
    [[nodiscard]] explicit SpatialSimulation(Layout &&layout,
                                             delay_t wire_delay_per_distance);

    [[nodiscard]] auto layout() const -> const Layout &;
    [[nodiscard]] auto schematic() const -> const Schematic &;
    [[nodiscard]] auto simulation() const -> const Simulation &;
    [[nodiscard]] auto simulation() -> Simulation &;

    [[nodiscard]] auto line_tree(wire_id_t wire_id) const -> const LineTree &;

    [[nodiscard]] auto wire_delay_per_distance() const -> delay_t;

   private:
    Layout layout_;
    std::vector<LineTree> line_trees_;

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
