#ifndef LOGIKSIM_SCHEMATIC_GENERATION_H
#define LOGIKSIM_SCHEMATIC_GENERATION_H

#include <vector>

namespace logicsim {

struct delay_t;

struct logicitem_id_t;
struct wire_id_t;
struct element_id_t;

class LineTree;
class Layout;
class Schematic;

auto add_missing_placeholders(Schematic& schematic) -> void;

/**
 * @brief: Generates a schematic from the given layout and line-trees.
 *
 * Pre-condition: All line-trees are equivalent to the segment-trees of the Layout.
 *
 * Throws an exception if inputs and output connections are not compatible.
 */
[[nodiscard]] auto generate_schematic(const Layout& layout,
                                      const std::vector<LineTree>& line_trees,
                                      delay_t wire_delay_per_distance) -> Schematic;

[[nodiscard]] auto to_element_id(const Layout& layout, logicitem_id_t logicitem_id)
    -> element_id_t;
[[nodiscard]] auto to_element_id(const Layout& layout, wire_id_t wire_id) -> element_id_t;

[[nodiscard]] auto to_logicitem_id(const Layout& layout, element_id_t element_id)
    -> logicitem_id_t;
[[nodiscard]] auto to_wire_id(const Layout& layout, element_id_t element_id) -> wire_id_t;

}  // namespace logicsim

#endif
