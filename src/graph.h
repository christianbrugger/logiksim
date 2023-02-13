#ifndef LOGIKSIM_GRAPH_H
#define LOGIKSIM_GRAPH_H

#include "exceptions.h"
#include "format.h"
#include "geometry.h"
#include "range.h"

#include <boost/container/static_vector.hpp>
#include <fmt/core.h>
#include <gsl/gsl>

#include <algorithm>
#include <optional>
#include <vector>

namespace logicsim {

[[nodiscard]] auto to_points_sorted_unique(std::ranges::input_range auto&& segments)
    -> std::vector<point_t> {
    auto points = std::vector<point_t> {};
    points.reserve(2 * std::size(segments));

    for (line_t segment : segments) {
        points.push_back(segment.p0);
        points.push_back(segment.p1);
    }

    std::ranges::sort(points);
    points.erase(std::ranges::unique(points).begin(), points.end());

    return points;
}

template <typename index_t>
class AdjacencyGraph {
   public:
    using key_type = index_t;
    using mapped_type = point_t;

    // each point can only have 4 neighbors without collisions
    using neighbor_t = boost::container::static_vector<key_type, 4>;
    using point_vector_t = std::vector<mapped_type>;
    using neighbor_vector_t = std::vector<neighbor_t>;

   public:
    AdjacencyGraph() = default;

    AdjacencyGraph(std::ranges::input_range auto&& segments)
        : points_ {to_points_sorted_unique(segments)} {
        neighbors_.resize(points_.size());

        for (line_t segment : segments) {
            add_edge_unchecked(segment);
        }

        // to make the representation deterministic
        sort_adjacency();
    }

    auto format() const -> std::string {
        return fmt::format("AdjacencyGraph(\n    points = {}\n    neighbors = {})\n",
                           points(), neighbors());
    }

    auto point(index_t vertex_id) const -> point_t {
        return points_.at(vertex_id);
    }

    auto points() const -> const point_vector_t& {
        return points_;
    }

    auto vertex_count() const -> index_t {
        return gsl::narrow_cast<index_t>(points_.size());
    }

    // iterator over all indices
    auto indices() const noexcept {
        return range(vertex_count());
    }

    auto neighbors() const -> const neighbor_vector_t& {
        return neighbors_;
    }

    auto to_index(point_t _point) const -> std::optional<index_t> {
        const auto res = std::ranges::lower_bound(points_, _point);
        if (res != points_.end()) {
            return gsl::narrow_cast<index_t>(res - points_.begin());
        }
        return std::nullopt;
    }

   private:
    // assumes point is part of graph
    auto to_index_unchecked(point_t _point) const -> size_t {
        auto found = std::ranges::lower_bound(points_, _point);
        assert(found != points_.end());
        return found - points_.begin();
    }

    // assumes both endpoints are part of graph
    auto add_edge_unchecked(line_t segment) -> void {
        if (segment.p0 == segment.p1) [[unlikely]] {
            throw_exception("Trying to add segment with zero length.");
        }

        auto index0 = to_index_unchecked(segment.p0);
        auto index1 = to_index_unchecked(segment.p1);

        auto& adjacency0 = neighbors_[index0];
        auto& adjacency1 = neighbors_[index1];

        if (std::ranges::find(adjacency0, index1) != adjacency0.end()) [[unlikely]] {
            throw_exception("Duplicate segments when building graph.");
        }

        if (adjacency0.size() == adjacency0.capacity()
            || adjacency1.size() == adjacency1.capacity()) [[unlikely]] {
            throw_exception(
                "Point has too many neighbors when building adjacency graph.");
        }

        adjacency0.push_back(gsl::narrow_cast<index_t>(index1));
        adjacency1.push_back(gsl::narrow_cast<index_t>(index0));
    }

    auto sort_adjacency() -> void {
        for (auto& adjacency : neighbors_) {
            std::ranges::sort(adjacency, {},
                              [&](index_t index) { return points_[index]; });
        }
    }

    point_vector_t points_ {};
    neighbor_vector_t neighbors_ {};
};

//
// Depth First Search
//

namespace detail {
template <typename index_t>
struct dfs_backtrack_memory_t {
    index_t graph_index;
    uint8_t neighbor_id;
};
}  // namespace detail

struct PrintingGraphVisitor {
    template <typename index_t>
    auto tree_edge(index_t a, index_t b, const AdjacencyGraph<index_t>& graph) {
        fmt::print("tree_edge: index {} {} - points {} {}\n", a, b, graph.point(a),
                   graph.point(b));
    }
};

// template <typename index_t, typename Func>
template <typename Func>
class TreeEdgeVisitor {
   public:
    explicit TreeEdgeVisitor(Func func) : func_ {std::move(func)} {}

    template <typename index_t>
    auto tree_edge(index_t a, index_t b, const AdjacencyGraph<index_t>& graph) {
        std::invoke(func_, a, b, graph);
    }

   private:
    Func func_;
};

template <typename index_t, typename length_t>
class LengthRecorderVisitor {
   public:
    using length_vector_t = std::vector<length_t>;

    LengthRecorderVisitor(index_t vertex_count) : length_vector_(vertex_count, 0) {}

    auto tree_edge(index_t a, index_t b, AdjacencyGraph<index_t> graph) -> void {
        auto line = line_t {graph.points().at(a), graph.points().at(b)};
        length_vector_.at(b) = length_vector_.at(a) + distance(line);
    };

    auto lengths() const -> const length_vector_t& {
        return length_vector_;
    }

    auto length(index_t vertex_id) const -> length_t {
        return length_vector_.at(vertex_id);
    }

   private:
    length_vector_t length_vector_ {};
};

template <typename... Ts>
struct combine_visitors {
    combine_visitors() = default;

    combine_visitors(Ts... args) : visitors {args...} {}

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

enum class DFSResult {
    success,
    unfinished_loop,
    unfinished_disconnected,
};

// visits all edges in the graph
template <typename index_t, class Visitor>
auto depth_first_search(const AdjacencyGraph<index_t>& graph, Visitor&& visitor,
                        index_t start) -> DFSResult {
    // memorize for loop detection
    auto visited = boost::container::vector<bool>(graph.points().size(), false);
    auto n_vertex_visited = index_t {1};

    auto found_loop = depth_first_visitor(
        start, visited,
        // discover_connections
        [&](index_t node, std::output_iterator<index_t> auto result) -> void {
            for (auto neighbor_id : reverse_range(graph.neighbors().at(node).size())) {
                *result = graph.neighbors()[node][neighbor_id];
                ++result;
            }
        },
        // visit_edge
        [&](index_t a, index_t b) -> void {
            visitor.tree_edge(a, b, graph);
            ++n_vertex_visited;
        });

    if (found_loop) {
        return DFSResult::unfinished_loop;
    }
    if (n_vertex_visited != graph.vertex_count()) {
        return DFSResult::unfinished_disconnected;
    }
    return DFSResult::success;
}

}  // namespace logicsim

#endif
