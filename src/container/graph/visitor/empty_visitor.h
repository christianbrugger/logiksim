#ifndef LOGICSIM_CONTAINER_GRAPH_VISITOR_EMPTY_VISITOR_H
#define LOGICSIM_CONTAINER_GRAPH_VISITOR_EMPTY_VISITOR_H

namespace logicsim {

template <typename index_t>
class AdjacencyGraph;

/**
 * @brief: Visitor which does nothing.
 *
 * Note this is useful, when one is only interested in the depth first search result.
 */
class EmptyVisitor {
   public:
    template <typename index_t>
    auto tree_edge(index_t, index_t, const AdjacencyGraph<index_t>&) const -> void {}
};

}  // namespace logicsim

#endif
