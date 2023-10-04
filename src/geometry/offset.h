#ifndef LOGICSIM_GEOMETRY_OFFSET_H
#define LOGICSIM_GEOMETRY_OFFSET_H

namespace logicsim {

struct grid_t;
struct offset_t;
struct ordered_line_t;
struct point_t;

[[nodiscard]] auto to_offset(grid_t x, grid_t reference) -> offset_t;
[[nodiscard]] auto to_grid(offset_t offset, grid_t reference) -> grid_t;

[[nodiscard]] auto to_offset(ordered_line_t line) -> offset_t;
[[nodiscard]] auto to_offset(ordered_line_t full_line, point_t point) -> offset_t;
[[nodiscard]] auto to_point(ordered_line_t full_line, offset_t offset) -> point_t;

}  // namespace logicsim

#endif
