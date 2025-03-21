#ifndef LOGICSIM_RANDOM_CONNECTION_COUNT_H
#define LOGICSIM_RANDOM_CONNECTION_COUNT_H

#include "core/random/generator.h"
#include "core/vocabulary/logicitem_type.h"

namespace logicsim {

struct connection_count_t;

[[nodiscard]] auto get_random_connection_count(
    Rng& rng, connection_count_t min, connection_count_t max) -> connection_count_t;

[[nodiscard]] auto get_random_input_count(Rng& rng, LogicItemType logicitem_type)
    -> connection_count_t;
[[nodiscard]] auto get_random_output_count(Rng& rng, LogicItemType logicitem_type)
    -> connection_count_t;

}  // namespace logicsim

#endif
