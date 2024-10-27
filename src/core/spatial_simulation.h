#ifndef LOGICSIM_SPATIAL_SIMULATION_H
#define LOGICSIM_SPATIAL_SIMULATION_H

#include "core/format/struct.h"
#include "core/layout.h"
#include "core/line_tree.h"
#include "core/simulation.h"

#include <vector>

namespace logicsim {

struct schematic_generation_result_t;

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
 *     + Segment trees in the layout have all cross-points & corners set.
 *     + Schematic is created with wire_delay_per_distance_ and it never changes.
 */
class SpatialSimulation {
   public:
    [[nodiscard]] explicit SpatialSimulation();

    /**
     * @brief: Create a new spatial simulation.
     *
     * Pre-condition: All inserted segment-trees are expected to form contiguous
     * trees. Pre-condition: All inserted segment-trees have all cross-points &
     * corners set.
     *
     * Note this can be achieved through methods in `tree_normalization.h`.
     */
    [[nodiscard]] explicit SpatialSimulation(Layout &&layout,
                                             delay_t wire_delay_per_distance);
    /**
     * @brief: Create a new spatial simulation from generation result.
     */
    [[nodiscard]] explicit SpatialSimulation(
        Layout &&layout, schematic_generation_result_t &&generation_result);

    // TODO: define equality
    // [[nodiscard]] auto operator==(const SpatialSimulation &) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;

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

// TODO: make regular
// static_assert(std::regular<SpatialSimulation>);
static_assert(std::semiregular<SpatialSimulation>);

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
