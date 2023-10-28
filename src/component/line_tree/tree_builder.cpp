#include "component/line_tree/tree_builder.h"

#include "component/line_tree/line_store.h"
#include "container/graph/adjacency_graph.h"
#include "container/graph/depth_first_search.h"
#include "container/graph/visitor/calling_visitor.h"
#include "vocabulary/line_index.h"

// #include "geometry/to_points_sorted_unique.h"
// #include "geometry/to_points_with_both_orientation.h

namespace logicsim {

namespace line_tree {

namespace {
using LineGraph = AdjacencyGraph<line_index_t::value_type>;

/*
auto split_lines(std::ranges::input_range auto&& segments,
             std::ranges::input_range auto&& points) -> std::vector<ordered_line_t> {
std::vector<ordered_line_t> result;
result.reserve(std::size(segments) + std::size(points));

auto splitter = SegmentSplitter {};
for (auto segment : segments) {
    std::ranges::copy(splitter.split_segment(segment, points),
                      std::back_inserter(result));
}
return result;
}

template <class OutputIterator, class GetterSame, class GetterDifferent>
auto merge_lines_1d(std::span<const ordered_line_t> segments, OutputIterator result,
                GetterSame get_same, GetterDifferent get_different) -> void {
// collect lines
auto parallel_segments = std::vector<ordered_line_t> {};
parallel_segments.reserve(segments.size());
transform_if(
    segments, std::back_inserter(parallel_segments),
    [&](ordered_line_t line) -> ordered_line_t { return line; },
    [&](ordered_line_t line) -> bool {
        return get_same(line.p0) == get_same(line.p1);
    });

// sort lists
std::ranges::sort(parallel_segments, [&](ordered_line_t a, ordered_line_t b) {
    return std::tie(get_same(a.p0), get_different(a.p0)) <
           std::tie(get_same(b.p0), get_different(b.p0));
});

// extract elements
transform_combine_while(
    parallel_segments, result,
    // make state
    [](auto it) -> ordered_line_t { return *it; },
    // combine while
    [&](ordered_line_t state, auto it) -> bool {
        return get_same(state.p0) == get_same(it->p0) &&
               get_different(state.p1) >= get_different(it->p0);
    },
    // update state
    [&](ordered_line_t state, auto it) -> ordered_line_t {
        get_different(state.p1) =
            std::max(get_different(state.p1), get_different(it->p1));
        return state;
    });
}

auto merge_lines(std::span<const ordered_line_t> segments)
-> std::vector<ordered_line_t> {
auto result = std::vector<ordered_line_t> {};
result.reserve(segments.size());

auto get_x = [](point_t& point) -> grid_t& { return point.x; };
auto get_y = [](point_t& point) -> grid_t& { return point.y; };

// vertical & horizontal
merge_lines_1d(segments, std::back_inserter(result), get_x, get_y);
merge_lines_1d(segments, std::back_inserter(result), get_y, get_x);

return result;
}

auto merge_split_segments(std::span<const ordered_line_t> segments)
-> std::vector<ordered_line_t> {
// merge
const auto segments_merged = merge_lines(segments);
// split
// TODO can this be simplified?
const auto points1 = to_points_sorted_unique(segments);
const auto segments_split = split_lines(segments_merged, points1);
const auto points2 = to_points_with_both_orientations(segments_split);
return split_lines(segments_merged, points2);
}

template <typename index_t>
auto select_best_root(const AdjacencyGraph<index_t>& graph,
                  std::optional<point_t> mandatory) -> std::optional<point_t> {
// collect candidates
auto root_candidates = std::vector<point_t> {};

auto to_point = [&](index_t index) { return graph.point(index); };
auto is_leaf = [&](index_t index) { return graph.neighbors()[index].size() == 1; };
transform_if(graph.indices(), std::back_inserter(root_candidates), to_point, is_leaf);

if (root_candidates.empty()) {
    // no root candiates
    return std::nullopt;
}

std::ranges::sort(root_candidates);
const auto has_candidate = [&](point_t _root) {
    return std::ranges::binary_search(root_candidates, _root);
};

// mandatory
if (mandatory) {
    if (!has_candidate(*mandatory)) [[unlikely]] {
        // requested root is not possible
        return std::nullopt;
    }
    return *mandatory;
}

// original line_tree roots
auto to_root = [](auto tree_reference) {
    return tree_reference.get().input_position();
};
if (auto result = std::ranges::find_if(line_trees, has_candidate, to_root);
    result != line_trees.end()) {
    return to_root(*result);
}

return root_candidates.at(0);
}

class TreeBuilderVisitor {
    using index_t = line_index_t::value_type;

   public:
    TreeBuilderVisitor(LineStore& store) : store_ {&store} {}

    auto tree_edge(index_t a, index_t b, const LineGraph& graph) -> void {
        const auto line = line_t {graph.point(a), graph.point(b)};
    };

   private:
    gsl::not_null<LineStore*> store_;
};
*/

auto create_line_store(point_t root, const LineGraph& graph) -> LineStore {
    auto line_store = LineStore {};

    const auto root_index = graph.to_index(root);
    if (!root_index) {
        throw std::runtime_error("root is not part of tree");
    }

    const auto edge_count = graph.vertex_count() - 1;
    line_store.reserve(edge_count);

    // from index of second point b -> line_index of segment that ends in point b
    auto last_index_lookup =
        std::vector<line_index_t>(graph.vertex_count(), null_line_index);
    const auto visitor = CallingVisitor {
        [&last_index_lookup, &line_store](
            LineGraph::index_type a, LineGraph::index_type b, const LineGraph& graph) {
            const auto line = line_t {graph.point(a), graph.point(b)};
            if (line_store.empty()) {
                last_index_lookup.at(b) = line_store.add_first_line(line);
            } else {
                last_index_lookup.at(b) =
                    line_store.add_line(line, last_index_lookup.at(a));
            }
        }};

    if (depth_first_search(graph, visitor, *root_index) == DFSStatus::success) {
        assert(std::ssize(line_store) == edge_count);
        line_store.shrink_to_fit();
        return line_store;
    }
    throw std::runtime_error("graph is not a tree");
}

/*
auto is_leaf(const LineGraph& graph, LineGraph::index_t index) -> bool {
    return graph.neighbors().at(index).size() == 1;
};

auto is_leaf(const LineGraph& graph, point_t point) -> bool {
    const auto index = graph.to_index(point);
    if (!index) [[unlikely]] {
        throw std::runtime_error("point is not part of graph");
    }
    return is_leaf(graph, *index);
}
*/

}  // namespace

// auto create_line_store_generic(std::span<const ordered_line_t> segments,
//                                std::optional<point_t> new_root) -> LineStore {
//     const auto merged_segments = merge_split_segments(segments);
//     const auto graph = LineGraph {merged_segments};
//
//     if (const auto root = select_best_root(graph, new_root)) {
//         return create_line_store(*root, graph);
//     }
//     throw std::runtime_error("invalid root");
// }

[[nodiscard]] auto create_line_store_simplified(std::span<const ordered_line_t> segments,
                                                point_t new_root) -> LineStore {
    const auto graph = LineGraph {segments};

    return create_line_store(new_root, graph);
}

}  // namespace line_tree

}  // namespace logicsim
