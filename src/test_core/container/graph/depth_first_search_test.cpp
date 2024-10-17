
#include "core/container/graph/depth_first_search.h"

#include "core/container/graph/adjacency_graph.h"
#include "core/container/graph/visitor/calling_visitor.h"
#include "core/vocabulary/line.h"
#include "core/vocabulary/point.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

TEST(Graph, DepthFirstSearchSimple) {
    using index_t = uint16_t;
    auto segments = {
        line_t {point_t {0, 0}, point_t {0, 1}},
        line_t {point_t {0, 1}, point_t {1, 1}},
        line_t {point_t {0, 0}, point_t {1, 0}},
    };
    auto graph = AdjacencyGraph<index_t>(segments);

    auto edges = std::vector<std::pair<index_t, index_t>> {};
    auto visitor =
        CallingVisitor([&](index_t a, index_t b, auto graph_ [[maybe_unused]]) {
            edges.push_back({a, b});
        });

    auto found_loop = depth_first_search<index_t>(graph, visitor, 0);

    EXPECT_THAT(found_loop, DFSStatus::success);
    ASSERT_THAT(edges.size(), 3);

    auto p0 = point_t {0, 0};
    auto p1 = point_t {0, 1};
    auto p2 = point_t {1, 1};
    auto p3 = point_t {1, 0};

    ASSERT_THAT(graph.point(edges.at(0).first), p0);
    ASSERT_THAT(graph.point(edges.at(0).second), p1);

    ASSERT_THAT(graph.point(edges.at(1).first), p1);
    ASSERT_THAT(graph.point(edges.at(1).second), p2);

    ASSERT_THAT(graph.point(edges.at(2).first), p0);
    ASSERT_THAT(graph.point(edges.at(2).second), p3);
}

}  // namespace logicsim