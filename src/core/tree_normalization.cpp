#include "core/tree_normalization.h"

#include "core/algorithm/compare_sorted.h"
#include "core/algorithm/transform_to_vector.h"
#include "core/container/graph/adjacency_graph.h"
#include "core/container/graph/depth_first_search.h"
#include "core/container/graph/visitor/empty_visitor.h"
#include "core/geometry/line.h"
#include "core/geometry/segment_info.h"
#include "core/geometry/to_points_sorted_unique.h"
#include "core/geometry/to_points_with_both_orientation.h"
#include "core/logging.h"
#include "core/segment_tree.h"

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

[[nodiscard]] auto find_root_index(const ValidationGraph& graph)
    -> std::optional<std::size_t> {
    const auto& indices = graph.indices();

    if (const auto it = std::ranges::find_if(
            indices, [&](std::size_t index) { return is_leaf(graph, index); });
        it != indices.end()) {
        return *it;
    }

    return std::nullopt;
}

}  // namespace

auto merge_split_segments(std::span<const ordered_line_t> segments)
    -> std::vector<ordered_line_t> {
    auto result = std::vector<ordered_line_t> {};
    result.reserve(segments.size());

    merge_lines_1d<Lines::horizontal>(segments, std::back_inserter(result));
    merge_lines_1d<Lines::vertical>(segments, std::back_inserter(result));

    return result;
}

namespace {

auto normalize_segments(std::span<const ordered_line_t> segments)
    -> std::vector<ordered_line_t> {
    // merge
    const auto segments_merged = merge_split_segments(segments);
    // split points
    const auto points1 = to_points_sorted_unique(segments);
    const auto segments_split = split_lines(segments_merged, points1);
    const auto points2 = to_points_with_both_orientations(segments_split);
    // split
    return split_lines(segments_merged, points2);
}

}  // namespace

auto segments_are_normalized(std::span<const ordered_line_t> segments) -> bool {
    if (segments.empty()) {
        return true;
    }

    auto normalized_segments = normalize_segments(segments);

    return normalized_segments.size() == segments.size() &&
           compare_sorted(std::move(normalized_segments),
                          std::vector<ordered_line_t> {segments.begin(), segments.end()});
};

namespace {

/**
 * @brief: check if graph is tree (detects loops && disconnected parts)
 */
auto graph_is_connected_tree(const ValidationGraph& graph) -> bool {
    const auto root_index = find_root_index(graph);
    if (!root_index) {
        return false;
    }

    return depth_first_search(graph, EmptyVisitor {}, *root_index) == DFSStatus::success;
}

auto segments_are_contiguous_tree(std::span<const ordered_line_t> segments,
                                  const ValidationGraph& graph) -> bool {
    return !segments.empty() &&               //
           graph_is_connected_tree(graph) &&  //
           segments_are_normalized(segments);
}

}  // namespace

auto segments_are_contiguous_tree(std::span<const ordered_line_t> segments) -> bool {
    const auto graph = ValidationGraph {segments};
    return segments_are_contiguous_tree(segments, graph);
}

auto is_contiguous_tree(const SegmentTree& tree) -> bool {
    const auto segments = transform_to_vector(all_lines(tree));
    return segments_are_contiguous_tree(segments);
}

namespace {

auto add_points_of_type(std::vector<point_t>& container, const SegmentTree& tree,
                        SegmentPointType query_type) -> void {
    for (const auto& info : tree.segments()) {
        for (const auto& [point, type] : to_point_type(info)) {
            if (type == query_type) {
                container.push_back(point);
            }
        }
    }
}

auto has_same_inputs_outputs(const SegmentTree& tree, const ValidationGraph& graph)
    -> bool {
    // tree
    auto tree_points = std::vector<point_t> {};
    add_points_of_type(tree_points, tree, SegmentPointType::input);
    add_points_of_type(tree_points, tree, SegmentPointType::output);

    assert(tree_points.size() ==
           std::size_t {tree.input_count()} + std::size_t {tree.output_count()});

    // graph
    auto graph_points = std::vector<point_t> {};
    for (const auto index : graph.indices()) {
        if (graph.neighbors(index).size() == 1) {
            graph_points.push_back(graph.point(index));
        }
    }

    return compare_sorted(std::move(tree_points), std::move(graph_points));
}

auto has_same_cross_points(const SegmentTree& tree, const ValidationGraph& graph)
    -> bool {
    // tree
    auto tree_points = std::vector<point_t> {};
    add_points_of_type(tree_points, tree, SegmentPointType::cross_point);

    // graph
    auto graph_points = std::vector<point_t> {};
    for (const auto index : graph.indices()) {
        if (graph.neighbors(index).size() >= 3) {
            graph_points.push_back(graph.point(index));
        }
    }

    return compare_sorted(std::move(tree_points), std::move(graph_points));
}

auto has_same_corner_points(const SegmentTree& tree, const ValidationGraph& graph)
    -> bool {
    // tree
    auto tree_points = std::vector<point_t> {};
    add_points_of_type(tree_points, tree, SegmentPointType::corner_point);

    // graph
    auto graph_points = std::vector<point_t> {};
    for (const auto index : graph.indices()) {
        if (is_corner(graph, index)) {
            graph_points.push_back(graph.point(index));
        }
    }

    return compare_sorted(std::move(tree_points), std::move(graph_points));
}

auto has_same_shadow_points(const SegmentTree& tree, const ValidationGraph& graph)
    -> bool {
    const auto shadow_point_allowed = [&](point_t point) -> bool {
        const auto index = graph.to_index(point).value();
        // allowed for corners and cross_point
        return is_corner(graph, index) || graph.neighbors(index).size() >= 3;
    };

    for (const auto& info : tree.segments()) {
        for (const auto& [point, type] : to_point_type(info)) {
            if (type == SegmentPointType::shadow_point && !shadow_point_allowed(point)) {
                return false;
            }
        }
    }

    return true;
}

auto has_no_unknown_points(const SegmentTree& tree) -> bool {
    for (const auto& info : tree.segments()) {
        for (const auto& [point, type] : to_point_type(info)) {
            if (type == SegmentPointType::new_unknown) {
                return false;
            }
        }
    }

    return true;
}

auto has_correct_endpoints(const SegmentTree& tree, const ValidationGraph& graph)
    -> bool {
    return has_same_inputs_outputs(tree, graph) &&  //
           has_same_cross_points(tree, graph) &&    //
           has_same_corner_points(tree, graph) &&   //
           has_same_shadow_points(tree, graph) &&   //
           has_no_unknown_points(tree);
}

}  // namespace

auto has_correct_endpoints(const SegmentTree& tree) -> bool {
    const auto graph = ValidationGraph {all_lines(tree)};

    return has_correct_endpoints(tree, graph);
}

auto is_contiguous_tree_with_correct_endpoints(const SegmentTree& tree) -> bool {
    const auto segments = transform_to_vector(all_lines(tree));
    const auto graph = ValidationGraph {segments};

    return !tree.empty() && segments_are_contiguous_tree(segments, graph) &&
           has_correct_endpoints(tree, graph);
}

}  // namespace logicsim
