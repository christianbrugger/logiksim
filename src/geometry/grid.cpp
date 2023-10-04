#include "geometry/grid.h"

#include "algorithm/round.h"
#include "vocabulary/grid.h"
#include "vocabulary/grid_fine.h"

#include <gsl/gsl>

namespace logicsim {

namespace {
auto clamp_discrete_to_grid(grid_fine_t grid_fine) -> grid_t {
    const auto clamped = clamp_to_grid(grid_fine);
    return grid_t {gsl::narrow_cast<grid_t::value_type>(double {clamped})};
}
}  // namespace

auto to_rounded(grid_fine_t v) -> grid_t {
    return clamp_discrete_to_grid(round(v));
}

auto to_floored(grid_fine_t v) -> grid_t {
    return clamp_discrete_to_grid(floor(v));
}

auto to_ceiled(grid_fine_t v) -> grid_t {
    return clamp_discrete_to_grid(ceil(v));
}

auto clamp_to_grid(grid_fine_t grid_fine) -> grid_fine_t {
    return std::clamp(grid_fine,                    //
                      grid_fine_t {grid_t::min()},  //
                      grid_fine_t {grid_t::max()});
}

auto round(grid_fine_t v) -> grid_fine_t {
    return grid_fine_t {round_fast(double {v})};
}

auto floor(grid_fine_t v) -> grid_fine_t {
    return grid_fine_t {std::floor(double {v})};
}

auto ceil(grid_fine_t v) -> grid_fine_t {
    return grid_fine_t {std::ceil(double {v})};
}

auto add_unchecked(grid_t grid, int delta) -> grid_t {
    return grid_t {gsl::narrow_cast<grid_t::value_type>(grid.value + delta)};
}

}  // namespace logicsim
