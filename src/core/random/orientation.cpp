#include "core/random/orientation.h"

#include "core/algorithm/random_select.h"
#include "core/algorithm/uniform_int_distribution.h"
#include "core/layout_info.h"
#include "core/vocabulary/direction_type.h"

#include <exception>

namespace logicsim {

auto get_random_orientation(Rng& rng) -> orientation_t {
    return *random_select(all_orientations, rng);
}

auto get_random_directed_orientation(Rng& rng) -> orientation_t {
    const auto orientation = get_random_orientation(rng);

    if (!is_directed(orientation)) {
        return get_random_directed_orientation(rng);
    }

    return orientation;
}

auto get_random_orientation(Rng& rng, LogicItemType logicitem_type) -> orientation_t {
    const auto type = element_direction_type(logicitem_type);

    switch (type) {
        using enum DirectionType;

        case undirected:
            return orientation_t::undirected;
        case directed:
            // return get_random_directed_orientation(rng);
            return orientation_t::right;
        case any:
            return get_random_orientation(rng);
    }
    std::terminate();
}

}  // namespace logicsim
