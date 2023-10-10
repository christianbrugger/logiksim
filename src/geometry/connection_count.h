#ifndef LOGICSIM_GEOMETRY_CONNECTION_COUNT_H
#define LOGICSIM_GEOMETRY_CONNECTION_COUNT_H

#include "vocabulary/connection_count.h"
#include "vocabulary/grid.h"
#include "vocabulary/grid_fine.h"

namespace logicsim {

[[nodiscard]] constexpr auto to_grid(connection_count_t count) -> grid_t;
[[nodiscard]] constexpr auto to_grid_fine(connection_count_t count) -> grid_fine_t;

//
// Implementation
//

constexpr auto to_grid(connection_count_t count) -> grid_t {
    return grid_t {count.count()};
}

constexpr auto to_grid_fine(connection_count_t count) -> grid_fine_t {
    return grid_fine_t {to_grid(count)};
}

}  // namespace logicsim

#endif
