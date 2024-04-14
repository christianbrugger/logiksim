#ifndef LOGICSIM_BENCHMARK_SCHEMATIC_CREATION_H
#define LOGICSIM_BENCHMARK_SCHEMATIC_CREATION_H

#include "random/generator.h"

namespace logicsim {

class Schematic;

namespace schematic_benchmark::defaults {
constexpr inline auto logic_element_count = 10'000;
}  // namespace schematic_benchmark::defaults

/**
 * @brief: Generate a schematic with n logic items.
 *
 * A linear circuit is returned consisting of a string of and and-elements and wires.
 * Each element has 2 inputs. The output is connected both inputs via a wire to the next.
 */
auto benchmark_schematic(
    int n_elements = schematic_benchmark::defaults::logic_element_count)
    -> Schematic;

}  // namespace logicsim

#endif
