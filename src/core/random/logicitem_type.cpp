#include "random/logicitem_type.h"

#include "algorithm/random_select.h"
#include "algorithm/to_underlying.h"
#include "algorithm/uniform_int_distribution.h"

namespace logicsim {

auto get_random_logicitem_type(Rng& rng) -> LogicItemType {
    const auto type = *random_select(all_logicitem_types, rng);

    if (type != LogicItemType::sub_circuit) {
        return type;
    }

    return get_random_logicitem_type(rng);
}

}  // namespace logicsim
