#include "core/random/insertion_mode.h"

#include "core/algorithm/uniform_int_distribution.h"

#include <exception>

namespace logicsim {

auto get_random_insertion_mode(Rng& rng) -> InsertionMode {
    using enum InsertionMode;

    int index = uint_distribution(0, 2)(rng);

    switch (index) {
        case 0:
            return temporary;
        case 1:
            return collisions;
        case 2:
            return insert_or_discard;

        default:
            std::terminate();
    }
}

}  // namespace logicsim
