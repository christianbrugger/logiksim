#ifndef LOGICSIM_BENCHMARK_SCHEMATIC_CREATION_H
#define LOGICSIM_BENCHMARK_SCHEMATIC_CREATION_H

#include "random/generator.h"
#include "random/schematic.h"

namespace logicsim {

class SchematicOld;

auto benchmark_schematic(int n_elements = random::defaults::schematic_element_count)
    -> SchematicOld;

}  // namespace logicsim

#endif
