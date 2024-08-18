#ifndef LOGICSIM_RANDOM_INSERTION_MODE_H
#define LOGICSIM_RANDOM_INSERTION_MODE_H

#include "random/generator.h"
#include "vocabulary/insertion_mode.h"

namespace logicsim {

[[nodiscard]] auto get_random_insertion_mode(Rng& rng) -> InsertionMode;

}

#endif
