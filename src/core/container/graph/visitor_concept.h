#ifndef LOGICSIM_CONTAINER_GRAPH_VISITOR_CONCEPT_H
#define LOGICSIM_CONTAINER_GRAPH_VISITOR_CONCEPT_H

namespace logicsim {

template <typename index_t>
class AdjacencyGraph;

/**
 * @brief: All the requirements for a depth first visitor.
 */
template <typename T, typename index_t>
concept dfs_visitor =
    requires(T visitor, index_t a, index_t b, const AdjacencyGraph<index_t>& graph) {
        visitor.tree_edge(a, b, graph);
    };

}  // namespace logicsim

#endif
