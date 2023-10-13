#ifndef LOGICSIM_RANDOM_INTERNAL_STATE_COUNT_H
#define LOGICSIM_RANDOM_INTERNAL_STATE_COUNT_H

#include "random/generator.h"
#include "vocabulary/element_type.h"

namespace logicsim {

[[nodiscard]] auto get_random_internal_state_count(Rng &rng, ElementType element_type)
    -> std::size_t;

}

#endif
