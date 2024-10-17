#include "core/random/grid.h"

#include "core/algorithm/uniform_int_distribution.h"
#include "core/vocabulary/grid.h"

namespace logicsim {

auto get_random_grid(Rng& rng) -> grid_t {
    return get_random_grid(rng, grid_t::min(), grid_t::max());
}

auto get_random_grid(Rng& rng, grid_t min, grid_t max) -> grid_t {
    return grid_t {uint_distribution(min.value, max.value)(rng)};
}

}  // namespace logicsim
