#ifndef LOGICSIM_RANDOM_BOOL_H
#define LOGICSIM_RANDOM_BOOL_H

#include "random/generator.h"

namespace logicsim {

[[nodiscard]] auto get_random_bool(Rng& rng) -> bool;
[[nodiscard]] auto get_random_bool(Rng& rng, double percentage) -> bool;

}

#endif
