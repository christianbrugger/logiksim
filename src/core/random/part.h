#ifndef LOGICSIM_RANDOM_PART_H
#define LOGICSIM_RANDOM_PART_H

#include "core/random/generator.h"

namespace logicsim {

struct part_t;
struct ordered_line_t;

[[nodiscard]] auto get_random_part(Rng& rng, part_t full_part) -> part_t;
[[nodiscard]] auto get_random_part(Rng& rng, ordered_line_t line) -> part_t;

}  // namespace logicsim

#endif
