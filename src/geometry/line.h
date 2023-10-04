#ifndef LOGICSIM_GEOMETRY_LINE_H
#define LOGICSIM_GEOMETRY_LINE_H

namespace logicsim {

struct grid_t;
struct point_t;
struct line_t;
struct ordered_line_t;

[[nodiscard]] auto distance(line_t line) -> int;
[[nodiscard]] auto distance(ordered_line_t line) -> int;

// order points within lines
[[nodiscard]] auto order_points(const line_t line0, const line_t line1) noexcept
    -> std::tuple<ordered_line_t, ordered_line_t>;

[[nodiscard]] auto is_endpoint(point_t point, line_t line) -> bool;
[[nodiscard]] auto is_endpoint(point_t point, ordered_line_t line) -> bool;

[[nodiscard]] auto add_unchecked(line_t line, int dx, int dy) -> line_t;
[[nodiscard]] auto add_unchecked(ordered_line_t line, int dx, int dy) -> ordered_line_t;

// is the point inside the line, excluding endpoints
[[nodiscard]] auto is_inside(point_t point, ordered_line_t line) noexcept -> bool;
// is the point colliding with the line
[[nodiscard]] auto is_colliding(point_t point, ordered_line_t line) noexcept -> bool;
// are the line endpoints colliding with the other line
[[nodiscard]] auto line_points_colliding(ordered_line_t line0,
                                         ordered_line_t line1) noexcept -> bool;

}  // namespace logicsim

#endif
