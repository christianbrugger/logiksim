
#include "graph.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

TEST(Graph, DepthFirstSearchSimple) {
    using index_t = uint16_t;
    auto segments = {
        line2d_t {{0, 0}, {0, 1}},
        line2d_t {{0, 1}, {1, 1}},
        line2d_t {{0, 0}, {1, 0}},
    };
    auto graph = AdjacencyGraph<index_t>(segments);

    auto edges = std::vector<std::pair<index_t, index_t>> {};
    auto visitor
        = TreeEdgeVisitor([&](index_t a, index_t b, auto graph_ [[maybe_unused]]) {
              edges.push_back({a, b});
          });

    auto found_loop = depth_first_search<index_t>(graph, visitor, 0);

    EXPECT_THAT(found_loop, DFSResult::success);
    ASSERT_THAT(edges.size(), 3);

    auto p0 = point2d_t {0, 0};
    auto p1 = point2d_t {0, 1};
    auto p2 = point2d_t {1, 1};
    auto p3 = point2d_t {1, 0};

    ASSERT_THAT(graph.point(edges.at(0).first), p0);
    ASSERT_THAT(graph.point(edges.at(0).second), p1);

    ASSERT_THAT(graph.point(edges.at(1).first), p1);
    ASSERT_THAT(graph.point(edges.at(1).second), p2);

    ASSERT_THAT(graph.point(edges.at(2).first), p0);
    ASSERT_THAT(graph.point(edges.at(2).second), p3);
}

}  // namespace logicsim