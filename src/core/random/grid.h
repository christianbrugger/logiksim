#ifndef LOGICSIM_RANDOM_GRID_H
#define LOGICSIM_RANDOM_GRID_H

#include "random/generator.h"

namespace logicsim {

struct grid_t;

[[nodiscard]] auto get_random_grid(Rng& rng) -> grid_t;
[[nodiscard]] auto get_random_grid(Rng& rng, grid_t min, grid_t max) -> grid_t;

}  // namespace logicsim

#endif
