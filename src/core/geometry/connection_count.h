#ifndef LOGICSIM_GEOMETRY_CONNECTION_COUNT_H
#define LOGICSIM_GEOMETRY_CONNECTION_COUNT_H

#include "core/algorithm/range_extended.h"
#include "core/vocabulary/connection_count.h"
#include "core/vocabulary/connection_id.h"
#include "core/vocabulary/grid.h"
#include "core/vocabulary/grid_fine.h"

namespace logicsim {

[[nodiscard]] constexpr auto to_grid(connection_count_t count) -> grid_t;
[[nodiscard]] constexpr auto to_grid_fine(connection_count_t count) -> grid_fine_t;

[[nodiscard]] auto first_id(connection_count_t count) -> connection_id_t;
[[nodiscard]] auto last_id(connection_count_t count) -> connection_id_t;
[[nodiscard]] auto id_range(connection_count_t count)
    -> range_extended_t<connection_id_t>;

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
