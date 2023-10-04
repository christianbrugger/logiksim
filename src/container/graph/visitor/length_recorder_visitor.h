#ifndef LOGICSIM_CONTAINER_GRAPH_VISITOR_LENGTH_RECORDER_VISITOR_H
#define LOGICSIM_CONTAINER_GRAPH_VISITOR_LENGTH_RECORDER_VISITOR_H

#include "container/graph/adjacency_graph.h"
#include "geometry.h"
#include "vocabulary/line.h"

#include <vector>

namespace logicsim {

/**
 * @brief: Visitor that stores the length from the root of each vertex.
 *
 * Note the result can be rethreaved from the visitor itself after the search.
 */
template <typename index_t = int, typename length_t = int>
class LengthRecorderVisitor {
   public:
    using length_vector_t = std::vector<length_t>;

    [[nodiscard]] explicit LengthRecorderVisitor(index_t vertex_count);

    auto tree_edge(index_t a, index_t b, AdjacencyGraph<index_t> graph) -> void;

    [[nodiscard]] auto lengths() const -> const length_vector_t&;
    [[nodiscard]] auto length(index_t vertex_id) const -> length_t;

   private:
    length_vector_t length_vector_ {};
};

//
// Implementation
//

template <typename index_t, typename length_t>
LengthRecorderVisitor<index_t, length_t>::LengthRecorderVisitor(index_t vertex_count)
    : length_vector_(vertex_count, 0) {}

template <typename index_t, typename length_t>
auto LengthRecorderVisitor<index_t, length_t>::tree_edge(index_t a, index_t b,
                                                         AdjacencyGraph<index_t> graph)
    -> void {
    auto line = line_t {graph.points().at(a), graph.points().at(b)};
    // TODO overflow check
    length_vector_.at(b) = length_vector_.at(a) + distance(line);
};

template <typename index_t, typename length_t>
auto LengthRecorderVisitor<index_t, length_t>::lengths() const -> const length_vector_t& {
    return length_vector_;
}

template <typename index_t, typename length_t>
auto LengthRecorderVisitor<index_t, length_t>::length(index_t vertex_id) const
    -> length_t {
    return length_vector_.at(vertex_id);
}

}  // namespace logicsim

#endif
