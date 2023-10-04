#ifndef LOGIKSIM_GRAPH_H
#define LOGIKSIM_GRAPH_H

#include "algorithm/depth_first_visitor.h"
#include "algorithm/range.h"
#include "container/graph/adjacency_graph.h"
#include "container/graph/depth_first_search.h"
#include "container/graph/visitor/empty_visitor.h"
#include "container/graph/visitor/length_recorder_visitor.h"
#include "exception.h"
#include "geometry.h"
#include "vocabulary.h"

#include <fmt/core.h>
#include <gsl/gsl>

#include <algorithm>
#include <optional>
#include <string>

namespace logicsim {

struct point_and_orientation_t {
    point_t point;
    bool is_horizontal;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const point_and_orientation_t& other) const
        -> bool = default;
    [[nodiscard]] auto operator<=>(const point_and_orientation_t& other) const = default;
};

[[nodiscard]] auto to_point_and_orientation(std::ranges::input_range auto&& lines)
    -> std::vector<point_and_orientation_t> {
    auto points = std::vector<point_and_orientation_t> {};
    points.reserve(2 * std::size(lines));

    for (auto line : lines) {
        const auto orientation = is_horizontal(line);

        points.push_back({line.p0, orientation});
        points.push_back({line.p1, orientation});
    }

    return points;
}

namespace detail {
// put in detail, as we are modifying the arguments
[[nodiscard]] auto extract_points_with_both_orientations(
    std::vector<point_and_orientation_t>& points) -> std::vector<point_t>;
}  // namespace detail

[[nodiscard]] auto points_with_both_orientations(std::ranges::input_range auto&& lines)
    -> std::vector<point_t> {
    auto points = to_point_and_orientation(lines);
    return detail::extract_points_with_both_orientations(points);
}

}  // namespace logicsim

#endif
