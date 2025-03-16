#ifndef LOGICSIM_ALGORITHM_DEPTH_FIRST_VISITOR_H
#define LOGICSIM_ALGORITHM_DEPTH_FIRST_VISITOR_H

#include "core/iterator_adaptor/transform_if_output_iterator.h"
#include "core/iterator_adaptor/transform_output_iterator.h"

#include <gsl/gsl>

#include <iterator>
#include <ranges>
#include <utility>
#include <vector>

namespace logicsim {

/**
 * @brief: depth first visitor
 *
 *  start_node:
 *      The index for which the algorithm should start the search.
 *
 *  visited_state:
 *      Datstructure to store the visited state. Should be initialized to false:
 *          visited_state[IndexType] = true;
 *          visited_state[IndexType] -> bool;
 *      Can be a vector for a continous integer index or in general an associative map.
 *
 *  discover_connection:
 *      From the given node add all connected nodes to the output iterator as IndexType.
 *
 *      auto discover_connection(IndexType node,
 *                               std::output_iterator<IndexType> auto result) -> void;
 *
 *  visit_edge:
 *      Called for each edge with signature:
 *
 *      auto visit_edge(IndexType a, IndexType b) -> void;
 *
 *  Returns true if it found a loop.
 */
template <typename VisitedStore, typename DiscoverConnected, typename EdgeVisitor,
          typename IndexType>
auto depth_first_visitor(IndexType start_node, VisitedStore& visited,
                         DiscoverConnected discover_connections,
                         EdgeVisitor visit_edge) -> bool {
    std::vector<std::pair<IndexType, IndexType>> edges_stack {};

    const auto result = transform_output_iterator(
        [=](IndexType second) { return std::make_pair(start_node, second); },
        std::back_inserter(edges_stack));
    discover_connections(start_node, result);
    visited.at(gsl::narrow<std::size_t>(start_node)) = true;

    while (true) {
        if (edges_stack.empty()) {
            return false;
        }
        const auto edge = edges_stack.back();
        edges_stack.pop_back();

        if (visited.at(gsl::narrow<std::size_t>(edge.second))) {
            // we abort on loops
            return true;
        }
        visited.at(gsl::narrow<std::size_t>(edge.second)) = true;

        visit_edge(edge.first, edge.second);
        discover_connections(
            edge.second,
            transform_if_output_iterator(
                [=](IndexType second) { return second != edge.first; },
                [=](IndexType second) { return std::make_pair(edge.second, second); },
                std::back_inserter(edges_stack)));
    }
}

}  // namespace logicsim

#endif
