#ifndef LOGIKSIM_SCHEMATIC_GENERATION_H
#define LOGIKSIM_SCHEMATIC_GENERATION_H

#include "layout.h"
#include "schematic_old.h"

namespace logicsim {

struct delay_t;
class LineTree;

auto calculate_output_delays(const LineTree& line_tree, delay_t wire_delay_per_distance)
    -> std::vector<delay_t>;

[[nodiscard]] auto generate_schematic(const Layout& layout, delay_t wire_delay_per_unit)
    -> SchematicOld;

}  // namespace logicsim

#endif
