#ifndef LOGIKSIM_GEOMETRY_H
#define LOGIKSIM_GEOMETRY_H

#include "vocabulary.h"

#include <tuple>

namespace logicsim {

//
// line_t
//

auto is_horizontal(line_t line) noexcept -> bool;
auto is_vertical(line_t line) noexcept -> bool;

// order points within lines
auto order_points(line_t line) noexcept -> line_t;
// order lines and points within lines
auto order_points(const line_t line0, const line_t line1) noexcept
    -> std::tuple<line_t, line_t>;

// fast distance for horitonal or vertical lines
auto distance(line_t line) -> int;

auto is_endpoint(point_t point, line_t line) -> bool;

//
// orientation_t
//

auto to_orientation(point_t p0, point_t p1) -> orientation_t;
auto to_orientation(line_t line) -> orientation_t;

// angle in respect to left orientation
auto to_angle(orientation_t orientation) -> double;

//
// offset_t
//

auto to_offset(grid_t x, grid_t reference) -> offset_t;
auto to_grid(offset_t offset, grid_t reference) -> grid_t;

//
// part_t
//

// TODO rename: to_part, to_line
auto to_part(line_t line) -> part_t;
auto to_part(line_t line, rect_fine_t rect) -> std::optional<part_t>;
auto to_line(line_t line, part_t part) -> line_t;

}  // namespace logicsim

#endif