#ifndef LOGICSIM_GEOMETRY_TO_POINTS_WITH_BOTH_ORIENTATION_H
#define LOGICSIM_GEOMETRY_TO_POINTS_WITH_BOTH_ORIENTATION_H

#include <span>
#include <vector>

namespace logicsim {

struct line_t;
struct ordered_line_t;
struct point_t;

[[nodiscard]] auto to_points_with_both_orientations(std::span<const line_t> lines)
    -> std::vector<point_t>;

[[nodiscard]] auto to_points_with_both_orientations(std::span<const ordered_line_t> lines)
    -> std::vector<point_t>;

}  // namespace logicsim

#endif
