
#include "line_tree.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>

namespace logicsim {

TEST(LineTree, DefaultCreation) {
    const auto tree = LineTree {};
    ASSERT_EQ(tree.validate(), true);
}

TEST(LineTree, ListCreation) {
    const auto tree = LineTree {{0, 0}, {10, 0}, {10, 12}};
    ASSERT_EQ(tree.validate(), true);
}

TEST(LineTree, TestSegments) {
    const auto tree = LineTree {{0, 0}, {10, 0}, {10, 12}};

    auto line0 = line2d_t {{0, 0}, {10, 0}};
    auto line1 = line2d_t {{10, 0}, {10, 12}};

    ASSERT_EQ(tree.segment_count(), 2);
    ASSERT_EQ(tree.segment(0), line0);
    ASSERT_EQ(tree.segment(1), line1);
}

TEST(LineTree, TestSegmentIterator) {
    const auto tree = LineTree {{0, 0}, {10, 0}, {10, 12}};

    auto line0 = line2d_t {{0, 0}, {10, 0}};
    auto line1 = line2d_t {{10, 0}, {10, 12}};

    ASSERT_THAT(tree.segments(), testing::ElementsAre(line0, line1));
}

TEST(LineTree, TestSizeSegmentIterator) {
    const auto tree = LineTree {{0, 0}, {10, 0}, {10, 12}, {20, 12}};

    auto line0 = LineTree::sized_line2d_t {line2d_t {{0, 0}, {10, 0}}, 0, 10};
    auto line1 = LineTree::sized_line2d_t {line2d_t {{10, 0}, {10, 12}}, 10, 22};
    auto line2 = LineTree::sized_line2d_t {line2d_t {{10, 12}, {20, 12}}, 22, 32};

    ASSERT_THAT(tree.sized_segments(), testing::ElementsAre(line0, line1, line2));
}

TEST(LineTree, TestIteratorNeighborTest) {
    const auto tree = LineTree {{0, 0}, {10, 0}, {10, 12}, {20, 12}};

    const auto it0 = tree.segments().begin();
    const auto it1 = std::next(it0);
    const auto it2 = std::next(it1);

    ASSERT_EQ(it0.is_connected(it0), false);
    ASSERT_EQ(it0.is_connected(it1), true);
    ASSERT_EQ(it0.is_connected(it2), false);

    ASSERT_EQ(it1.is_connected(it0), true);
    ASSERT_EQ(it1.is_connected(it1), false);
    ASSERT_EQ(it1.is_connected(it2), true);

    ASSERT_EQ(it2.is_connected(it0), false);
    ASSERT_EQ(it2.is_connected(it1), true);
    ASSERT_EQ(it2.is_connected(it2), false);
}

TEST(LineTree, CreateWithDiagonalEdges) {
    EXPECT_THROW(LineTree({{0, 0}, {5, 5}}), InvalidLineTreeException);
    EXPECT_THROW(LineTree({{0, 0}, {0, 10}, {5, 5}}), InvalidLineTreeException);
}

TEST(LineTree, CreateWithUnecessaryPoints) {
    EXPECT_THROW(LineTree({{0, 0}, {0, 2}, {0, 4}}), InvalidLineTreeException);
    EXPECT_THROW(LineTree({{0, 0}, {0, 2}, {2, 2}, {4, 2}}), InvalidLineTreeException);
}

TEST(LineTree, CreateWithDuplicates) {
    EXPECT_THROW(LineTree({{0, 0}, {0, 10}, {10, 10}, {10, 0}, {0, 0}}),
                 InvalidLineTreeException);
}

TEST(LineTree, CreateWithCollisions) {
    EXPECT_THROW(LineTree({{0, 0}, {0, 10}, {0, 5}}), InvalidLineTreeException);
    EXPECT_THROW(LineTree({{0, 0}, {0, 10}, {0, -5}}), InvalidLineTreeException);
}

}  // namespace logicsim