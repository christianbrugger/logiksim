#ifndef LOGICSIM_RANDOM_SCHEMATIC_H
#define LOGICSIM_RANDOM_SCHEMATIC_H

#include "random/generator.h"

namespace logicsim {

class SchematicOld;

namespace random::defaults {
constexpr inline auto schematic_element_count = 100;
constexpr inline auto schematic_connectivity = 0.75;
}  // namespace random::defaults

auto create_random_schematic(
    Rng &rng, int n_elements = random::defaults::schematic_element_count,
    double connection_ratio = random::defaults::schematic_connectivity) -> SchematicOld;

}  // namespace logicsim

#endif
