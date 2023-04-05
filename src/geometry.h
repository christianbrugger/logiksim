#ifndef LOGIKSIM_GEOMETRY_H
#define LOGIKSIM_GEOMETRY_H

#include "vocabulary.h"

#include <tuple>

namespace logicsim {
auto is_horizontal(line_t line) noexcept -> bool;

auto is_vertical(line_t line) noexcept -> bool;

// order points within lines
auto order_points(line_t line) noexcept -> line_t;

// order lines and points within lines
auto order_points(const line_t line0, const line_t line1) noexcept
    -> std::tuple<line_t, line_t>;

// fast distance for horitonal or vertical lines
auto distance(line_t line) -> int;

auto to_orientation(point_t p0, point_t p1) -> orientation_t;
auto to_orientation(line_t line) -> orientation_t;

// angle in respect to left orientation
auto to_angle(orientation_t orientation) -> double;

auto is_endpoint(point_t point, line_t line) -> bool;

//
// Segments
//

auto get_segment_part(line_t line) -> part_t;

auto get_segment_part(line_t line, rect_fine_t rect) -> std::optional<part_t>;

// TODO rename
auto get_selected_segment(line_t segment, part_t selection) -> line_t;

}  // namespace logicsim

#endif