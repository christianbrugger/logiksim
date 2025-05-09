#ifndef LOGICSIM_GEOMETRY_TO_POINTS_SORTED_UNIQUE_H
#define LOGICSIM_GEOMETRY_TO_POINTS_SORTED_UNIQUE_H

#include "core/algorithm/make_unique.h"
#include "core/concept/input_range.h"
#include "core/vocabulary/line.h"
#include "core/vocabulary/ordered_line.h"
#include "core/vocabulary/point.h"

#include <span>
#include <vector>

namespace logicsim {

/**
 * @brief: Convert all line endpoints to a sorted vectors of unique points.
 */
template <typename R>
    requires input_range_of2<R, line_t, ordered_line_t>
auto to_points_sorted_unique(const R& segments) -> std::vector<point_t> {
    auto points = std::vector<point_t> {};
    points.reserve(std::size_t {2} * std::size(segments));

    for (auto segment : segments) {
        points.push_back(segment.p0);
        points.push_back(segment.p1);
    }

    sort_and_make_unique(points);
    return points;
}

}  // namespace logicsim

#endif
