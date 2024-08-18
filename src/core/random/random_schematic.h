#ifndef LOGICSIM_RANDOM_RANDOM_SCHEMATIC_H
#define LOGICSIM_RANDOM_RANDOM_SCHEMATIC_H

#include "random/generator.h"

namespace logicsim {

class Schematic;

namespace random::defaults {
constexpr inline auto schematic_element_count = 100;
constexpr inline auto schematic_connectivity = 0.75;
}  // namespace random::defaults

/**
 * @brief: Create a random schematic with n elements and given connectivity ratio.
 *
 * The elements are:
 *    * xor_element  (1-8 inputs)
 *    * inverters
 *    * wires        (1-8 outputs)
 *
 * Note that the resulting circuit might or might not contain loops.
 */
[[nodiscard]] auto create_random_schematic(
    Rng &rng, int n_elements = random::defaults::schematic_element_count,
    double connection_ratio = random::defaults::schematic_connectivity) -> Schematic;

[[nodiscard]] auto with_custom_delays(Rng &rng,
                                      const Schematic &schematic_orig) -> Schematic;

}  // namespace logicsim

#endif
