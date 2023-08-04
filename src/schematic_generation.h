#ifndef LOGIKSIM_SCHEMATIC_GENERATION_H
#define LOGIKSIM_SCHEMATIC_GENERATION_H

#include "layout.h"
#include "schematic.h"

namespace logicsim {

[[nodiscard]] auto generate_schematic(const Layout& layout, delay_t wire_delay_per_unit)
    -> Schematic;

}

#endif
