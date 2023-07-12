#ifndef LOGIKSIM_SCHEMATIC_GENERATION_H
#define LOGIKSIM_SCHEMATIC_GENERATION_H

#include "layout.h"
#include "schematic.h"

namespace logicsim {

[[nodiscard]] auto generate_schematic(const Layout& layout) -> Schematic;

}

#endif
