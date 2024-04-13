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
    static auto tree_edge(index_t /*unused*/, index_t /*unused*/,
                          const AdjacencyGraph<index_t>& /*unused*/) -> void {}
};

}  // namespace logicsim

#endif
