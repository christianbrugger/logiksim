#include "core/random/internal_state_count.h"

namespace logicsim {

auto get_random_internal_state_count(Rng &rng [[maybe_unused]],
                                     LogicItemType logicitem_type [[maybe_unused]])
    -> std::size_t {
    return 0;
}

}  // namespace logicsim
