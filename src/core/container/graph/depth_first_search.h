#ifndef LOGICSIM_CONTAINER_GRAPH_DEPTH_FIRST_SEARCH_H
#define LOGICSIM_CONTAINER_GRAPH_DEPTH_FIRST_SEARCH_H

#include "core/algorithm/depth_first_visitor.h"
#include "core/container/graph/adjacency_graph.h"
#include "core/container/graph/visitor_concept.h"
#include "core/format/enum.h"
#include "core/format/struct.h"

namespace logicsim {

/**
 * @brief: Termination status of a depth first search.
 *
 * success                 - algorithm visited all nodes
 * unfinished_loop         - algorithm stopped, because a loop was found
 * unfinished_disconnected - algorithm finished, but could not reach all nodes
 */
enum class DFSStatus {
    success,
    unfinished_loop,
    unfinished_disconnected,
};

template <>
[[nodiscard]] auto format(DFSStatus result) -> std::string;

/**
 * @brief: Result of the depth first search.
 *
 * visited             - bool mask with all visited nodes marked true
 * n_vertex_visited    - number of vertices visited
 * status              - the overall termination status
 */
template <typename index_t = int>
struct DFSResult {
    using visited_vector_t = boost::container::vector<bool>;

    visited_vector_t visited {};
    index_t n_vertex_visited {};
    DFSStatus status {DFSStatus::success};

    [[nodiscard]] auto format() const -> std::string;
};

template <typename index_t>
auto DFSResult<index_t>::format() const -> std::string {
    return fmt::format(
        "DFSResult(\n"
        "    visited = {}\n"
        "    n_vertex_visited = {}\n"
        "    status ={}\n"
        ")",
        visited, n_vertex_visited, status);
}

/**
 * @brief:  Visits all edges in the graph with the visitor from the start index.
 *
 * @param graph: the adjacency graph visited
 * @param vistor: the visitor who is called.
 * @param start: start index of the search
 *
 * The Visitor needs to fulfill the dfs_visitor concept.
 */
template <typename index_t, dfs_visitor<index_t> Visitor>
auto depth_first_search_visited(const AdjacencyGraph<index_t>& graph, Visitor&& visitor,
                                index_t start) -> DFSResult<index_t> {
    // memorize for loop detection
    auto result = DFSResult<index_t> {
        .visited =
            typename DFSResult<index_t>::visited_vector_t(graph.points().size(), false),
        .n_vertex_visited = index_t {1},
        .status = DFSStatus::success,
    };

    const auto found_loop = depth_first_visitor(
        start, result.visited,
        // discover_connections
        [&](index_t node, std::output_iterator<index_t> auto output) -> void {
            const auto node_s = gsl::narrow<std::size_t>(node);
            for (auto neighbor_id : reverse_range(graph.neighbors().at(node_s).size())) {
                *output = graph.neighbors()[node_s][neighbor_id];
                ++output;
            }
        },
        // visit_edge
        [&](index_t a, index_t b) -> void {
            visitor.tree_edge(a, b, graph);
            ++result.n_vertex_visited;
        });

    if (found_loop) {
        result.status = DFSStatus::unfinished_loop;
    } else if (result.n_vertex_visited != graph.vertex_count()) {
        result.status = DFSStatus::unfinished_disconnected;
    }

    return result;
}

/**
 * @brief:  Visits all edges in the graph with the visitor from the start index.
 *
 * @param graph: the adjacency graph visited
 * @param vistor: the visitor who is called.
 * @param start: start index of the search
 *
 * The Visitor needs to fulfill the dfs_visitor concept.
 */
template <typename index_t, dfs_visitor<index_t> Visitor>
auto depth_first_search(const AdjacencyGraph<index_t>& graph, Visitor&& visitor,
                        index_t start) -> DFSStatus {
    return depth_first_search_visited(graph, std::forward<Visitor>(visitor), start)
        .status;
}

}  // namespace logicsim

#endif
