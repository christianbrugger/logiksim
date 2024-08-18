#include "random/connection_count.h"

#include "algorithm/uniform_int_distribution.h"
#include "layout_info.h"
#include "vocabulary/connection_count.h"

namespace logicsim {

auto get_random_connection_count(Rng& rng, connection_count_t min,
                                 connection_count_t max) -> connection_count_t {
    return connection_count_t {uint_distribution(min.count(), max.count())(rng)};
}

auto get_random_input_count(Rng& rng,
                            LogicItemType logicitem_type) -> connection_count_t {
    const auto min = element_input_count_min(logicitem_type);
    const auto max = element_input_count_min(logicitem_type);

    return get_random_connection_count(rng, min, max);
}

auto get_random_output_count(Rng& rng,
                             LogicItemType logicitem_type) -> connection_count_t {
    const auto min = element_output_count_min(logicitem_type);
    const auto max = element_output_count_min(logicitem_type);

    return get_random_connection_count(rng, min, max);
}

}  // namespace logicsim
