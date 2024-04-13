#ifndef LOGICSIM_CONTAINER_GRAPH_VISITOR_PRINTING_VISITOR_H
#define LOGICSIM_CONTAINER_GRAPH_VISITOR_PRINTING_VISITOR_H

#include "container/graph/adjacency_graph.h"

namespace logicsim {

template <typename index_t>
class AdjacencyGraph;

/**
 * @brief: Logs the visited edge.
 *
 * Note this is useful for debugging.
 */
struct PrintingGraphVisitor {
    template <typename index_t>
    static auto tree_edge(index_t a, index_t b, const AdjacencyGraph<index_t>& graph)
        -> void;
};

//
// Implementation
//

template <typename index_t>
auto PrintingGraphVisitor::tree_edge(index_t a, index_t b,
                                     const AdjacencyGraph<index_t>& graph) -> void {
    print_fmt("tree_edge: index {} {} - points {} {}\n", a, b, graph.point(a),
              graph.point(b));
}

}  // namespace logicsim

#endif
