#ifndef LOGIKSIM_SCHEMATIC_GENERATION_H
#define LOGIKSIM_SCHEMATIC_GENERATION_H

namespace logicsim {

struct delay_t;
class Layout;
class Schematic;
class SchematicOld;

[[nodiscard]] auto generate_schematic(const Layout& layout,
                                      delay_t wire_delay_per_distance)
    -> Schematic;

[[nodiscard]] auto generate_schematic(Schematic&& schematic,
                                      delay_t wire_delay_per_distance) -> SchematicOld;

}  // namespace logicsim

#endif
