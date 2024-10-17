#ifndef LOGICSIM_RANDOM_INTERNAL_STATE_COUNT_H
#define LOGICSIM_RANDOM_INTERNAL_STATE_COUNT_H

#include "core/random/generator.h"
#include "core/vocabulary/logicitem_type.h"

namespace logicsim {

[[nodiscard]] auto get_random_internal_state_count(Rng &rng, LogicItemType logicitem_type)
    -> std::size_t;

}

#endif
