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

template <typename index_t>
class AdjacencyGraph {
   public:
    // each point can only have 4 neighbors without collisions
    using neighbor_t = boost::container::static_vector<index_t, 4>;

    using point_vector_t = std::vector<point2d_t>;
    using neighbor_vector_t = std::vector<neighbor_t>;

   public:
    AdjacencyGraph() = default;

    AdjacencyGraph(std::ranges::input_range auto&& segments) {
        // add points
        points_.reserve(2 * std::size(segments));
        for (line2d_t segment : segments) {
            points_.push_back(segment.p0);
            points_.push_back(segment.p1);
        }

        // remove duplicates & sort
        std::ranges::sort(points_);
        points_.erase(std::ranges::unique(points_).begin(), points_.end());

        auto to_index = [&](point2d_t _point) {
            return std::ranges::lower_bound(points_, _point) - points_.begin();
        };

        // create adjacency
        neighbors_.resize(points_.size());
        for (line2d_t segment : segments) {
            if (segment.p0 == segment.p1) [[unlikely]] {
                throw_exception("Trying to add segment with zero length.");
            }

            auto index0 = to_index(segment.p0);
            auto index1 = to_index(segment.p1);

            auto& adjacency0 = neighbors_[index0];
            auto& adjacency1 = neighbors_[index1];

            if (std::ranges::find(adjacency0, index0) != adjacency0.end()) [[unlikely]] {
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

        // sort adjacency to make the representation deterministic
        for (auto& adjacency : neighbors_) {
            std::ranges::sort(adjacency, {},
                              [&](index_t index) { return points_[index]; });
        }
    }

    auto points() const -> const point_vector_t& {
        return points_;
    }

    auto neighbors() const -> const neighbor_vector_t& {
        return neighbors_;
    }

    // iterator over all indices
    auto indices() const noexcept {
        return range(points_.size());
    }

    auto find(point2d_t _point) const -> std::optional<index_t> {
        const auto res = std::ranges::lower_bound(points_, _point);
        if (res != points_.end()) {
            return gsl::narrow_cast<index_t>(res - points_.begin());
        }
        return std::nullopt;
    }

   private:
    point_vector_t points_ {};
    neighbor_vector_t neighbors_ {};
};

}  // namespace logicsim

template <typename index_t>
struct fmt::formatter<logicsim::AdjacencyGraph<index_t>> {
    static constexpr auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::AdjacencyGraph<index_t>& obj,
                       fmt::format_context& ctx) {
        return fmt::format_to(ctx.out(),
                              "AdjacencyGraph(\n    points = {}\n    neighbors = {})\n",
                              obj.points(), obj.neighbors());
    }
};

namespace logicsim {

//
// Depth First Search
//

namespace {
template <typename index_t>
struct dfs_backtrack_memory_t {
    index_t graph_index;
    uint8_t neighbor_id;
};
}  // namespace

enum class DFSResult {
    success,
    found_loop,
};

template <typename index_t>
struct PrintingGraphVisitor {
    auto tree_edge(index_t a, index_t b) {
        fmt::print("{} {}\n", a, b);
    };
};

// visits all edges in the graph
template <typename index_t, class Visitor>
[[nodiscard]] auto depth_first_search(const AdjacencyGraph<index_t>& graph,
                                      Visitor& visitor, index_t start) -> DFSResult {
    // memorize for loop detection
    boost::container::vector<bool> visited(graph.points().size(), false);
    // unhandeled outgoing edges
    std::vector<dfs_backtrack_memory_t<index_t>> backtrack_vector {};
    auto backtrack_index = size_t {0};

    for (auto neighbor_id : range(graph.neighbors().at(start).size())) {
        backtrack_vector.push_back(
            {.graph_index = start,
             .neighbor_id = gsl::narrow_cast<uint8_t>(neighbor_id)});
    }

    auto index_a = index_t {0};
    auto index_b = std::optional<index_t> {};

    while (true) {
        if (!index_b) {
            if (backtrack_index >= backtrack_vector.size()) {
                return DFSResult::success;
            }

            // load next backtracking
            auto backtrack = backtrack_vector[backtrack_index++];

            index_a = backtrack.graph_index;
            index_b = graph.neighbors()[backtrack.graph_index][backtrack.neighbor_id];
        }
        assert(index_b);

        // visit edge
        visitor.tree_edge(index_a, *index_b);

        // mark visited
        if (visited[*index_b]) {
            // graph contains loops
            return DFSResult::found_loop;
        }
        visited[*index_b] = true;

        // find outgoing edge
        auto& neighbors = graph.neighbors()[*index_b];
        auto next = std::optional<index_t> {};
        if (neighbors.at(0) == index_a) {
            if (neighbors.size() > 1) {
                next = neighbors[1];
            }
        } else {
            next = neighbors[0];
        }

        // add backtracking candiates
        for (auto id : range(neighbors.size())) {
            if (neighbors[id] != index_a && (!next || neighbors[id] != *next)) {
                backtrack_vector.push_back(
                    {.graph_index = *index_b,
                     .neighbor_id = gsl::narrow_cast<uint8_t>(id)});
            }
        }

        // choose where to go next
        index_a = index_b.value_or(0);
        index_b = next;
    }
}

inline auto test_graph() -> void {
    using index_t = uint16_t;
    auto segments = {
        line2d_t {{0, 0}, {0, 1}},
        line2d_t {{0, 1}, {1, 1}},
        line2d_t {{0, 0}, {1, 0}},
    };
    auto graph = AdjacencyGraph<index_t>(segments);
    fmt::print("graph = {}\n", graph);

    auto visitor = PrintingGraphVisitor<index_t> {};
    auto res = depth_first_search<index_t>(graph, visitor, 0);

    fmt::print("DFSResult::success = {}\n", res == DFSResult::success);
}
}  // namespace logicsim

#endif
