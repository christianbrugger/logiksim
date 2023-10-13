#ifndef LOGICSIM_RANDOM_POINT_H
#define LOGICSIM_RANDOM_POINT_H

#include "random/generator.h"

namespace logicsim {

struct grid_t;
struct point_t;
struct rect_t;

[[nodiscard]] auto get_random_point(Rng& rng) -> point_t;
[[nodiscard]] auto get_random_point(Rng& rng, grid_t min, grid_t max) -> point_t;
[[nodiscard]] auto get_random_point(Rng& rng, rect_t rect) -> point_t;

}  // namespace logicsim

#endif
