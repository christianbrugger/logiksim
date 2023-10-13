#include "random/element_type.h"

#include "algorithm/random_select.h"
#include "algorithm/to_underlying.h"
#include "algorithm/uniform_int_distribution.h"

namespace logicsim {

auto get_random_logic_item_type(Rng& rng) -> ElementType {
    const auto type = *random_select(all_element_types, rng);

    if (is_logic_item(type) && type != ElementType::sub_circuit) {
        return type;
    }

    return get_random_logic_item_type(rng);
}

}  // namespace logicsim
