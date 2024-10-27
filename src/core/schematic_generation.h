#ifndef LOGIKSIM_SCHEMATIC_GENERATION_H
#define LOGIKSIM_SCHEMATIC_GENERATION_H

#include "core/format/struct.h"
#include "core/line_tree.h"
#include "core/schematic.h"

#include <vector>

namespace logicsim {

struct delay_t;

struct logicitem_id_t;
struct wire_id_t;
struct element_id_t;

class Layout;

auto add_missing_placeholders(Schematic& schematic) -> void;

struct schematic_generation_result_t {
    std::vector<LineTree> line_trees;
    Schematic schematic;
    delay_t wire_delay_per_distance;

    [[nodiscard]] auto operator==(const schematic_generation_result_t&) const
        -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

/**
 * @brief: Generates a schematic from the given layout and line-trees.
 *
 * Pre-condition: All layout segment-trees are contiguous tree with correct endpoints.
 *                See `tree_normalization.h`.
 *
 * Throws an exception if inputs and output connections are not compatible.
 */
[[nodiscard]] auto generate_schematic(const Layout& layout,
                                      delay_t wire_delay_per_distance)
    -> schematic_generation_result_t;

[[nodiscard]] auto to_element_id(const Layout& layout,
                                 logicitem_id_t logicitem_id) -> element_id_t;
[[nodiscard]] auto to_element_id(const Layout& layout, wire_id_t wire_id) -> element_id_t;

[[nodiscard]] auto to_logicitem_id(const Layout& layout,
                                   element_id_t element_id) -> logicitem_id_t;
[[nodiscard]] auto to_wire_id(const Layout& layout, element_id_t element_id) -> wire_id_t;

}  // namespace logicsim

#endif
