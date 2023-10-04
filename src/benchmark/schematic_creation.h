#ifndef LOGICSIM_BENCHMARK_SCHEMATIC_CREATION_H
#define LOGICSIM_BENCHMARK_SCHEMATIC_CREATION_H

#include "schematic.h"

namespace logicsim {

inline constexpr int BENCHMARK_DEFAULT_ELEMENTS {100};
inline constexpr double BENCHMARK_DEFAULT_CONNECTIVITY {0.75};

auto benchmark_schematic(int n_elements = BENCHMARK_DEFAULT_ELEMENTS) -> Schematic;

template <std::uniform_random_bit_generator G>
auto create_random_schematic(G &rng, int n_elements = BENCHMARK_DEFAULT_ELEMENTS,
                             double connection_ratio = BENCHMARK_DEFAULT_CONNECTIVITY)
    -> Schematic;

}  // namespace logicsim

#endif
