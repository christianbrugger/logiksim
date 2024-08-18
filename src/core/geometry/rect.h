#ifndef LOGICSIM_GEOMETRY_RECT_H
#define LOGICSIM_GEOMETRY_RECT_H

namespace logicsim {

struct grid_fine_t;
struct point_t;
struct point_fine_t;
struct ordered_line_t;
struct rect_t;
struct rect_fine_t;

[[nodiscard]] auto enclosing_rect(rect_fine_t rect) -> rect_t;
[[nodiscard]] auto enclosing_rect(rect_t a, rect_t b) -> rect_t;
[[nodiscard]] auto enclosing_rect(rect_t rect, ordered_line_t line) -> rect_t;

[[nodiscard]] auto to_rect(point_fine_t center, grid_fine_t size) -> rect_fine_t;
[[nodiscard]] auto get_center(rect_fine_t rect) -> point_fine_t;

[[nodiscard]] auto is_colliding(point_t point, rect_t rect) noexcept -> bool;
[[nodiscard]] auto is_colliding(point_fine_t point, rect_fine_t rect) noexcept -> bool;

[[nodiscard]] auto is_colliding(ordered_line_t line, rect_t rect) noexcept -> bool;
[[nodiscard]] auto is_colliding(ordered_line_t line, rect_fine_t rect) noexcept -> bool;

[[nodiscard]] auto is_colliding(rect_fine_t a, rect_fine_t b) noexcept -> bool;
[[nodiscard]] auto is_colliding(rect_t a, rect_t b) noexcept -> bool;

}  // namespace logicsim

#endif
