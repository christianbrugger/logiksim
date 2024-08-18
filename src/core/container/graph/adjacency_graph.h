#ifndef LOGICSIM_CONTAINER_GRAPH_ADJACENCY_GRAPH_H
#define LOGICSIM_CONTAINER_GRAPH_ADJACENCY_GRAPH_H

#include "algorithm/narrow_integral.h"
#include "algorithm/range.h"
#include "concept/input_range.h"
#include "format/container.h"
#include "format/struct.h"
#include "geometry/line.h"
#include "geometry/to_points_sorted_unique.h"
#include "vocabulary/point.h"

#include <boost/container/static_vector.hpp>
#include <fmt/core.h>

#include <algorithm>
#include <cassert>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

namespace logicsim {

/**
 * @brief: A graph that stores for each vertex the connected vertices.
 *
 * Note that this class is not fully generic and tied to our vocabulary.
 */
template <typename index_t = int>
class AdjacencyGraph {
   public:
    using index_type = index_t;
    using key_type = index_t;
    using mapped_type = point_t;

    // each point can only have 4 neighbors, for orthogonal lines
    using neighbor_t = boost::container::static_vector<key_type, 4>;
    using point_vector_t = std::vector<mapped_type>;
    using neighbor_vector_t = std::vector<neighbor_t>;

   public:
    AdjacencyGraph() = default;

    template <typename R>
        requires input_range_of2<R, line_t, ordered_line_t>
    [[nodiscard]] explicit AdjacencyGraph(const R& segments);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto vertex_count() const -> index_t;
    [[nodiscard]] auto point(index_t vertex_id) const -> point_t;
    [[nodiscard]] auto points() const -> const point_vector_t&;

    [[nodiscard]] auto indices() const noexcept -> forward_range_t<index_t>;
    // TODO add index_t vertex_id. Test at() performance impact
    [[nodiscard]] auto neighbors() const -> const neighbor_vector_t&;
    [[nodiscard]] auto neighbors(index_t vertex_id) const -> const neighbor_t&;
    [[nodiscard]] auto to_index(point_t _point) const -> std::optional<index_t>;

   private:
    // assumes point is part of graph
    [[nodiscard]] auto to_index_unchecked(point_t _point) const -> std::size_t;
    // assumes both endpoints are part of graph
    auto add_edge_unchecked(const line_t& segment) -> void;
    auto sort_adjacency() -> void;

   private:
    point_vector_t points_ {};
    neighbor_vector_t neighbors_ {};
};

//
// Free Functions
//

template <typename index_t>
[[nodiscard]] auto is_leaf(const AdjacencyGraph<index_t>& graph,
                           index_t vertex_id) -> bool;

template <typename index_t>
[[nodiscard]] auto is_corner(const AdjacencyGraph<index_t>& graph,
                             index_t vertex_id) -> bool;

//
// Implementation
//

template <typename index_t>
template <typename R>
    requires input_range_of2<R, line_t, ordered_line_t>
AdjacencyGraph<index_t>::AdjacencyGraph(const R& segments)
    : points_ {to_points_sorted_unique(segments)} {
    neighbors_.resize(points_.size());

    for (auto segment : segments) {
        add_edge_unchecked(line_t {segment});
    }

    // to normalize the representation
    sort_adjacency();
}

template <typename index_t>
auto AdjacencyGraph<index_t>::format() const -> std::string {
    return fmt::format("AdjacencyGraph(\n    points = {}\n    neighbors = {})\n", points_,
                       neighbors_);
}

template <typename index_t>
auto AdjacencyGraph<index_t>::point(index_t vertex_id) const -> point_t {
    return points_.at(vertex_id);
}

template <typename index_t>
auto AdjacencyGraph<index_t>::points() const -> const point_vector_t& {
    return points_;
}

template <typename index_t>
auto AdjacencyGraph<index_t>::vertex_count() const -> index_t {
    return gsl::narrow_cast<index_t>(points_.size());
}

// iterator over all indices
template <typename index_t>
auto AdjacencyGraph<index_t>::indices() const noexcept -> forward_range_t<index_t> {
    return range(vertex_count());
}

template <typename index_t>
auto AdjacencyGraph<index_t>::neighbors() const -> const neighbor_vector_t& {
    return neighbors_;
}

template <typename index_t>
auto AdjacencyGraph<index_t>::neighbors(index_t vertex_id) const -> const neighbor_t& {
    return neighbors_.at(vertex_id);
}

template <typename index_t>
auto AdjacencyGraph<index_t>::to_index(point_t _point) const -> std::optional<index_t> {
    const auto res = std::ranges::lower_bound(points_, _point);
    if (res != points_.end()) {
        return gsl::narrow_cast<index_t>(res - points_.begin());
    }
    return std::nullopt;
}

// assumes point is part of graph
template <typename index_t>
auto AdjacencyGraph<index_t>::to_index_unchecked(point_t _point) const -> std::size_t {
    auto found = std::ranges::lower_bound(points_, _point);
    assert(found != points_.end());
    return found - points_.begin();
}

// assumes both endpoints are part of graph
template <typename index_t>
auto AdjacencyGraph<index_t>::add_edge_unchecked(const line_t& segment) -> void {
    auto index0 = to_index_unchecked(segment.p0);
    auto index1 = to_index_unchecked(segment.p1);

    auto& adjacency0 = neighbors_[index0];
    auto& adjacency1 = neighbors_[index1];

    if (std::ranges::find(adjacency0, index1) != adjacency0.end()) [[unlikely]] {
        throw std::runtime_error("Duplicate segments when building graph.");
    }

    if (adjacency0.size() == adjacency0.capacity() ||
        adjacency1.size() == adjacency1.capacity()) [[unlikely]] {
        throw std::runtime_error(
            "Point has too many neighbors when building adjacency graph.");
    }

    adjacency0.push_back(gsl::narrow_cast<index_t>(index1));
    adjacency1.push_back(gsl::narrow_cast<index_t>(index0));
}

template <typename index_t>
auto AdjacencyGraph<index_t>::sort_adjacency() -> void {
    for (auto& adjacency : neighbors_) {
        std::ranges::sort(adjacency, {}, [&](index_t index) { return points_[index]; });
    }
}

//
// Free Functions
//

template <typename index_t>
auto is_leaf(const AdjacencyGraph<index_t>& graph, index_t vertex_id) -> bool {
    return graph.neighbors(vertex_id).size() == 1;
}

template <typename index_t>
auto is_corner(const AdjacencyGraph<index_t>& graph, index_t vertex_id) -> bool {
    const auto& neighbors = graph.neighbors(vertex_id);

    if (neighbors.size() != 2) {
        return false;
    }

    const auto point = graph.point(vertex_id);
    return lines_orthogonal(line_t {point, graph.point(neighbors[0])},
                            line_t {point, graph.point(neighbors[1])});
}

}  // namespace logicsim

#endif
