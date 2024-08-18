#ifndef LOGICSIM_CONTAINER_GRAPH_COMBINE_VISITOR_H
#define LOGICSIM_CONTAINER_GRAPH_COMBINE_VISITOR_H

#include <tuple>

namespace logicsim {

template <typename index_t>
class AdjacencyGraph;

/**
 * @brief: Struct able to combine multiple visitors.
 */
template <typename... Ts>
struct combine_visitors {
    combine_visitors() = default;

    explicit combine_visitors(Ts... args) : visitors {args...} {}

    template <typename index_t>
    auto tree_edge(index_t a, index_t b, const AdjacencyGraph<index_t>& graph) {
        std::apply([&](auto&... visitor) { (visitor.tree_edge(a, b, graph), ...); },
                   visitors);
    }

    template <typename index_t>
    auto tree_edge(index_t a, index_t b, const AdjacencyGraph<index_t>& graph) const {
        std::apply([&](auto&... visitor) { (visitor.tree_edge(a, b, graph), ...); },
                   visitors);
    }

   private:
    std::tuple<Ts...> visitors;
};

}  // namespace logicsim

#endif
