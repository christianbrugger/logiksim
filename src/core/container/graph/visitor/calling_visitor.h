#ifndef LOGICSIM_CONTAINER_GRAPH_VISITOR_CALLING_VISITOR_H
#define LOGICSIM_CONTAINER_GRAPH_VISITOR_CALLING_VISITOR_H

#include <functional>
#include <utility>

namespace logicsim {

template <typename index_t>
class AdjacencyGraph;

/**
 * @brief: Calls the given function on each visited tree edge.
 *
 * Func: [](index_t a, index_t b, const AdjacencyGraph<index_t>& graph) -> void {};
 */
template <typename Func>
class CallingVisitor {
   public:
    explicit CallingVisitor(Func func) : func_ {std::move(func)} {}

    template <typename index_t>
    auto tree_edge(index_t a, index_t b, const AdjacencyGraph<index_t>& graph) const
        -> void;

   private:
    Func func_;
};

//
// Implementation
//

template <typename Func>
template <typename index_t>
auto CallingVisitor<Func>::tree_edge(index_t a, index_t b,
                                     const AdjacencyGraph<index_t>& graph) const -> void {
    std::invoke(func_, a, b, graph);
}

}  // namespace logicsim

#endif
