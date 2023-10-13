#include "random/orientation.h"

#include "algorithm/random_select.h"
#include "algorithm/uniform_int_distribution.h"
#include "layout_info.h"
#include "vocabulary/direction_type.h"

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

auto get_random_orientation(Rng& rng, ElementType element_type) -> orientation_t {
    const auto type = element_direction_type(element_type);

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
