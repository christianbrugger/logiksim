#ifndef LOGICSIM_GEOMETRY_GRID_H
#define LOGICSIM_GEOMETRY_GRID_H

namespace logicsim {

struct grid_fine_t;
struct grid_t;
struct connection_count_t;

[[nodiscard]] auto is_representable(int x, int y) -> bool;
[[nodiscard]] auto is_representable(grid_fine_t x, grid_fine_t y) -> bool;

[[nodiscard]] auto add_unchecked(grid_t grid, int delta) -> grid_t;

[[nodiscard]] auto to_rounded(grid_fine_t v) -> grid_t;
[[nodiscard]] auto to_floored(grid_fine_t v) -> grid_t;
[[nodiscard]] auto to_ceiled(grid_fine_t v) -> grid_t;

[[nodiscard]] auto clamp_to_grid(grid_fine_t grid_fine) -> grid_fine_t;
[[nodiscard]] auto round(grid_fine_t v) -> grid_fine_t;
[[nodiscard]] auto floor(grid_fine_t v) -> grid_fine_t;
[[nodiscard]] auto ceil(grid_fine_t v) -> grid_fine_t;

}  // namespace logicsim

#endif
