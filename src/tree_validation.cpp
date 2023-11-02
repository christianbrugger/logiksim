#include "tree_validation.h"

#include "container/graph/adjacency_graph.h"
#include "container/graph/depth_first_search.h"
#include "container/graph/visitor/empty_visitor.h"
#include "geometry/line.h"
#include "geometry/to_points_sorted_unique.h"
#include "geometry/to_points_with_both_orientation.h"
#include "logging.h"

#include <folly/small_vector.h>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/view/adjacent_remove_if.hpp>
#include <range/v3/view/partial_sum.hpp>

namespace logicsim {

namespace {

using ValidationGraph = AdjacencyGraph<std::size_t>;

using ordered_lines_t = folly::small_vector<ordered_line_t, 16>;

[[nodiscard]] auto split_segment(ordered_line_t segment,
                                 std::ranges::input_range auto&& points)
    -> ordered_lines_t {
    auto result = ordered_lines_t {};
    result.push_back(segment);

    for (auto point : points) {
        auto splittable = std::ranges::find_if(
            result,
            [&point](ordered_line_t line) -> bool { return is_inside(point, line); });

        if (splittable != result.end()) {
            const auto p0 = splittable->p0;
            const auto p1 = splittable->p1;

            *splittable = ordered_line_t {p0, point};
            result.push_back(ordered_line_t {point, p1});
        }
    }

    return result;
}

[[nodiscard]] auto split_lines(std::ranges::input_range auto&& segments,
                               std::ranges::input_range auto&& points)
    -> std::vector<ordered_line_t> {
    std::vector<ordered_line_t> result;
    result.reserve(std::size(segments) + std::size(points));

    for (auto segment : segments) {
        std::ranges::copy(split_segment(segment, points), std::back_inserter(result));
    }
    return result;
}

enum class Lines { horizontal, vertical };

template <Lines which, class OutputIterator>
auto merge_lines_1d(std::span<const ordered_line_t> segments, OutputIterator result)
    -> void {
    constexpr static auto X = which == Lines::horizontal ? &point_t::x : &point_t::y;
    constexpr static auto Y = which == Lines::horizontal ? &point_t::y : &point_t::x;

    auto parallel_segments = std::vector<ordered_line_t> {};
    parallel_segments.reserve(segments.size());

    // copy horizontal lines
    std::ranges::copy_if(
        segments, std::back_inserter(parallel_segments),
        [&](ordered_line_t line) -> bool { return line.p0.*Y == line.p1.*Y; });

    // sort in Y then X
    std::ranges::sort(parallel_segments, [&](ordered_line_t a, ordered_line_t b) {
        return std::tie(a.p0.*Y, a.p0.*X) < std::tie(b.p0.*Y, b.p0.*X);
    });

    // merge overlapping lines (with same Y)
    const auto overlapping_union = [&](ordered_line_t a,
                                       ordered_line_t b) -> ordered_line_t {
        if (a.p0.*Y == b.p0.*Y && a.p1.*X >= b.p0.*X) {
            a.p1.*X = std::max(a.p1.*X, b.p1.*X);
            return a;
        }
        return b;
    };
    const auto same_beginning = [](const ordered_line_t& a,
                                   const ordered_line_t& b) -> bool {
        return a.p0 == b.p0;
    };
    ranges::copy(ranges::views::partial_sum(parallel_segments, overlapping_union) |
                     ranges::views::adjacent_remove_if(same_beginning),
                 result);
}

[[nodiscard]] auto merge_lines(std::span<const ordered_line_t> segments)
    -> std::vector<ordered_line_t> {
    auto result = std::vector<ordered_line_t> {};
    result.reserve(segments.size());

    merge_lines_1d<Lines::horizontal>(segments, std::back_inserter(result));
    merge_lines_1d<Lines::vertical>(segments, std::back_inserter(result));

    return result;
}

/**
 * @brief: Finds normalized segments through splitting and merging.
 *
 * Overlapping or connecting & parallel segments are merged.
 * Lines with crossing points are split.
 *
 * Returns a flat list of segments, where there are no internal colliding points.
 */
[[nodiscard]] auto normalize_segments(std::span<const ordered_line_t> segments)
    -> std::vector<ordered_line_t> {
    // merge
    const auto segments_merged = merge_lines(segments);
    // split points
    const auto points1 = to_points_sorted_unique(segments);
    const auto segments_split = split_lines(segments_merged, points1);
    const auto points2 = to_points_with_both_orientations(segments_split);
    // split
    return split_lines(segments_merged, points2);
}

[[nodiscard]] auto find_root_index(const ValidationGraph& graph)
    -> std::optional<size_t> {
    auto is_leaf = [&](size_t index) { return graph.neighbors()[index].size() == 1; };

    for (auto index : graph.indices()) {
        if (is_leaf(index)) {
            return index;
        }
    }

    return std::nullopt;
}

}  // namespace

auto segments_are_contiguous_tree(std::vector<ordered_line_t>&& segments) -> bool {
    if (segments.empty()) {
        return true;
    }
    // normalize
    auto normalized_segments = normalize_segments(segments);

    // compare
    std::ranges::sort(segments);
    std::ranges::sort(normalized_segments);
    if (segments != normalized_segments) {
        return false;
    }

    // build graph
    const auto graph = ValidationGraph {segments};

    // find root
    const auto root_index = find_root_index(graph);
    if (!root_index) {
        return false;
    }

    // check if graph is tree (detects loops && disconnected parts)
    return depth_first_search(graph, EmptyVisitor {}, *root_index) == DFSStatus::success;
}

}  // namespace logicsim
