#ifndef LOGIKSIM_SCHEMATIC_GENERATION_H
#define LOGIKSIM_SCHEMATIC_GENERATION_H

namespace logicsim {

struct delay_t;
class Layout;
class Schematic;

auto add_missing_placeholders(Schematic& schematic) -> void;

[[nodiscard]] auto generate_schematic(const Layout& layout,
                                      delay_t wire_delay_per_distance) -> Schematic;

}  // namespace logicsim

#endif
