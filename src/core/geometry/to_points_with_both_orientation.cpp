#include "core/geometry/to_points_with_both_orientation.h"

#include "core/algorithm/copy_adjacent_if.h"
#include "core/algorithm/make_unique.h"
#include "core/format/struct.h"
#include "core/geometry/orientation.h"
#include "core/vocabulary/line.h"
#include "core/vocabulary/ordered_line.h"
#include "core/vocabulary/point.h"

#include <fmt/core.h>

#include <algorithm>

namespace logicsim {

namespace {

struct point_and_orientation_t {
    point_t point;
    bool is_horizontal;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const point_and_orientation_t& other) const
        -> bool = default;
    [[nodiscard]] auto operator<=>(const point_and_orientation_t& other) const
        -> std::strong_ordering = default;
};

inline auto point_and_orientation_t::format() const -> std::string {
    return fmt::format("{} {}", point, is_horizontal ? "horizontal" : "vertical");
}

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

[[nodiscard]] auto extract_points_with_both_orientations(
    std::vector<point_and_orientation_t>& points) -> std::vector<point_t>;

auto extract_points_with_both_orientations(std::vector<point_and_orientation_t>& points)
    -> std::vector<point_t> {
    sort_and_make_unique(points);

    auto result = std::vector<point_t> {};
    result.reserve(points.size());

    copy_adjacent_if(points.begin(), points.end(), std::back_inserter(result), {},
                     [](point_and_orientation_t item) -> point_t { return item.point; });
    return result;
}

[[nodiscard]] auto to_points_with_both_orientations_generic(
    std::ranges::input_range auto&& lines) -> std::vector<point_t> {
    auto points = to_point_and_orientation(lines);
    return extract_points_with_both_orientations(points);
}

}  // namespace

[[nodiscard]] auto to_points_with_both_orientations(std::span<const line_t> lines)
    -> std::vector<point_t> {
    return to_points_with_both_orientations_generic(lines);
}

[[nodiscard]] auto to_points_with_both_orientations(std::span<const ordered_line_t> lines)
    -> std::vector<point_t> {
    return to_points_with_both_orientations_generic(lines);
}

}  // namespace logicsim
