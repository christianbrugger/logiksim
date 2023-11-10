#include "component/line_tree/tree_builder.h"

#include "component/line_tree/line_store.h"
#include "container/graph/adjacency_graph.h"
#include "container/graph/depth_first_search.h"
#include "container/graph/visitor/calling_visitor.h"
#include "tree_normalization.h"
#include "vocabulary/line_index.h"
#include "vocabulary/ordered_line.h"
#include "vocabulary/point.h"

#include <cassert>
#include <stdexcept>

namespace logicsim {

namespace line_tree {

namespace {

using LineGraph = AdjacencyGraph<line_index_t::value_type>;

auto create_line_store(point_t root, const LineGraph& graph) -> LineStore {
    const auto root_index = graph.to_index(root);
    if (!root_index) {
        throw std::runtime_error("root is not part of tree");
    }

    const auto edge_count = graph.vertex_count() - 1;
    auto line_store = LineStore {};
    line_store.reserve(edge_count);

    // index of b -> line_index_t of line with endpoint b
    auto last_indices = std::vector<line_index_t>(graph.vertex_count(), null_line_index);
    using index_t = LineGraph::index_type;

    const auto visitor = CallingVisitor {
        [&last_indices, &line_store](index_t a, index_t b, const LineGraph& graph) {
            const auto line = line_t {graph.point(a), graph.point(b)};

            if (line_store.empty()) {
                last_indices.at(b) = line_store.add_first_line(line);
            } else {
                last_indices.at(b) = line_store.add_line(line, last_indices.at(a));
            }
        }};

    if (depth_first_search(graph, visitor, *root_index) == DFSStatus::success) {
        assert(std::ssize(line_store) == edge_count);
        line_store.shrink_to_fit();
        return line_store;
    }

    throw std::runtime_error("graph is not a tree");
}

}  // namespace

auto create_line_store(std::span<const ordered_line_t> segments, point_t new_root)
    -> LineStore {
    assert(segments_are_contiguous_tree(
        std::vector<ordered_line_t> {segments.begin(), segments.end()}));

    const auto graph = LineGraph {segments};
    return create_line_store(new_root, graph);
}

}  // namespace line_tree

}  // namespace logicsim
