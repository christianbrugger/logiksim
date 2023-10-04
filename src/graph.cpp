#include "graph.h"

#include "algorithm/copy_adjacent_if.h"

namespace logicsim {

auto point_and_orientation_t::format() const -> std::string {
    return fmt::format("{} {}", point, is_horizontal ? "horizontal" : "vertical");
}

namespace detail {
auto extract_points_with_both_orientations(std::vector<point_and_orientation_t>& points)
    -> std::vector<point_t> {
    auto result = std::vector<point_t> {};
    result.reserve(points.size());

    std::ranges::sort(points);
    points.erase(std::ranges::unique(points).begin(), points.end());

    copy_adjacent_if(points.begin(), points.end(), std::back_inserter(result), {},
                     [](point_and_orientation_t item) -> point_t { return item.point; });
    return result;
}
}  // namespace detail

template <>
auto format(DFSStatus result) -> std::string {
    switch (result) {
        using enum DFSStatus;

        case success:
            return "success";
        case unfinished_loop:
            return "unfinished_loop";
        case unfinished_disconnected:
            return "unfinished_disconnected";
    }
    throw_exception("unknown DFSStatus value");
}

}  // namespace logicsim
