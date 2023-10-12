#ifndef LOGICSIM_GEOMETRY_ORIENTATION_H
#define LOGICSIM_GEOMETRY_ORIENTATION_H

#include "vocabulary/orientation.h"

namespace logicsim {

struct point_t;
struct line_t;
struct ordered_line_t;

[[nodiscard]] auto is_horizontal(orientation_t orientation) noexcept -> bool;
[[nodiscard]] auto is_vertical(orientation_t orientation) noexcept -> bool;

[[nodiscard]] auto is_horizontal(point_t p0, point_t p1) noexcept -> bool;
[[nodiscard]] auto is_vertical(point_t p0, point_t p1) noexcept -> bool;

[[nodiscard]] auto is_horizontal(line_t line) noexcept -> bool;
[[nodiscard]] auto is_vertical(line_t line) noexcept -> bool;

[[nodiscard]] auto is_horizontal(ordered_line_t line) noexcept -> bool;
[[nodiscard]] auto is_vertical(ordered_line_t line) noexcept -> bool;

// from p0 to p1
[[nodiscard]] auto to_orientation(point_t p0, point_t p1) -> orientation_t;
[[nodiscard]] auto to_orientation_p0(ordered_line_t line) -> orientation_t;
[[nodiscard]] auto to_orientation_p1(ordered_line_t line) -> orientation_t;

// angle in respect to left orientation
[[nodiscard]] auto to_angle(orientation_t orientation) -> double;

[[nodiscard]] auto orientations_compatible(orientation_t a, orientation_t b) -> bool;

}  // namespace logicsim

#endif
