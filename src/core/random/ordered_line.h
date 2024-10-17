#ifndef LOGICSIM_RANDOM_ORDERED_LINE_H
#define LOGICSIM_RANDOM_ORDERED_LINE_H

#include "core/random/generator.h"

#include <vector>

namespace logicsim {

struct grid_t;
struct ordered_line_t;

[[nodiscard]] auto get_random_ordered_line(Rng& rng, grid_t min,
                                           grid_t max) -> ordered_line_t;
[[nodiscard]] auto get_random_ordered_line(Rng& rng, grid_t min, grid_t max,
                                           grid_t max_length) -> ordered_line_t;

[[nodiscard]] auto get_random_ordered_lines(Rng& rng, std::size_t count, grid_t min,
                                            grid_t max) -> std::vector<ordered_line_t>;

}  // namespace logicsim

#endif
