
#include "line_tree.h"

#include "format.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>

namespace logicsim {

TEST(LineTree, DefaultCreation) {
    const auto tree = LineTree {};
    ASSERT_EQ(tree.segment_count(), 0);
}

TEST(LineTree, ListCreation) {
    const auto tree = LineTree {{0, 0}, {10, 0}, {10, 12}};
    ASSERT_EQ(tree.segment_count(), 2);
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

    EXPECT_THROW(LineTree({{0, 0}, {0, 10}, {5, 10}, {5, 5}, {0, 5}}),
                 InvalidLineTreeException);
    EXPECT_THROW(LineTree({{0, 0}, {0, 10}, {10, 10}, {10, 0}, {-10, 0}}),
                 InvalidLineTreeException);
}

//
// Merge
//

TEST(LineTree, MergeTreesSimple) {
    auto tree1 = LineTree({{0, 0}, {0, 10}});
    auto tree2 = LineTree({{0, 10}, {10, 10}});

    auto line0 = LineTree::sized_line2d_t {line2d_t {{0, 0}, {0, 10}}, 0, 10};
    auto line1 = LineTree::sized_line2d_t {line2d_t {{0, 10}, {10, 10}}, 10, 20};

    auto tree = merge({tree1, tree2});

    ASSERT_EQ(tree.has_value(), true);
    EXPECT_THAT(tree->sized_segments(), testing::ElementsAre(line0, line1));
}

TEST(LineTree, MergeTreesLongChain) {
    auto tree1 = LineTree({{0, 0}, {0, 10}, {10, 10}, {10, 0}});
    auto tree2 = LineTree({{10, 0}, {20, 0}, {20, 10}, {30, 10}, {30, 0}});

    auto line0 = LineTree::sized_line2d_t {line2d_t {{0, 0}, {0, 10}}, 0, 10};
    auto line1 = LineTree::sized_line2d_t {line2d_t {{0, 10}, {10, 10}}, 10, 20};
    auto line2 = LineTree::sized_line2d_t {line2d_t {{10, 10}, {10, 0}}, 20, 30};
    auto line3 = LineTree::sized_line2d_t {line2d_t {{10, 0}, {20, 0}}, 30, 40};
    auto line4 = LineTree::sized_line2d_t {line2d_t {{20, 0}, {20, 10}}, 40, 50};
    auto line5 = LineTree::sized_line2d_t {line2d_t {{20, 10}, {30, 10}}, 50, 60};
    auto line6 = LineTree::sized_line2d_t {line2d_t {{30, 10}, {30, 0}}, 60, 70};

    auto tree = merge({tree1, tree2});

    ASSERT_EQ(tree.has_value(), true);
    EXPECT_THAT(tree->sized_segments(),
                testing::ElementsAre(line0, line1, line2, line3, line4, line5, line6));
}

TEST(LineTree, MergeTreesLongChainInverter) {
    auto tree1 = LineTree({{0, 0}, {0, 10}, {10, 10}, {10, 0}});
    auto tree2 = LineTree({{10, 0}, {20, 0}, {20, 10}, {30, 10}, {30, 0}});

    auto line0 = LineTree::sized_line2d_t {line2d_t {{30, 0}, {30, 10}}, 0, 10};
    auto line1 = LineTree::sized_line2d_t {line2d_t {{30, 10}, {20, 10}}, 10, 20};
    auto line2 = LineTree::sized_line2d_t {line2d_t {{20, 10}, {20, 0}}, 20, 30};
    auto line3 = LineTree::sized_line2d_t {line2d_t {{20, 0}, {10, 0}}, 30, 40};
    auto line4 = LineTree::sized_line2d_t {line2d_t {{10, 0}, {10, 10}}, 40, 50};
    auto line5 = LineTree::sized_line2d_t {line2d_t {{10, 10}, {0, 10}}, 50, 60};
    auto line6 = LineTree::sized_line2d_t {line2d_t {{0, 10}, {0, 0}}, 60, 70};

    auto tree = merge({tree1, tree2}, point2d_t {30, 0});

    ASSERT_EQ(tree.has_value(), true);
    EXPECT_THAT(tree->sized_segments(),
                testing::ElementsAre(line0, line1, line2, line3, line4, line5, line6));
}

TEST(LineTree, MergeNoRoot) {
    auto tree1 = LineTree({{0, 0}, {0, 10}, {10, 10}});
    auto tree2 = LineTree({{0, 0}, {10, 0}, {10, 10}});

    auto tree = merge({tree1, tree2});
    ASSERT_EQ(tree, std::nullopt);
}

TEST(LineTree, MergeWithLoop) {
    auto tree1 = LineTree({{0, 0}, {0, 10}, {10, 10}, {10, 0}});
    auto tree2 = LineTree({{10, 0}, {20, 0}, {20, 10}, {10, 10}});

    auto tree = merge({tree1, tree2});
    ASSERT_EQ(tree, std::nullopt);
}

// TEST(LineTree, MergeTwoSidesLoop) {
//     auto tree1 = LineTree({{1, 0}, {2, 0}, {2, 1}, {3, 1}, {3, 0}, {4, 0}});
//     auto tree2 = LineTree({{0, 0}, {4, 0}});
//
//     auto tree = merge({tree1, tree2});
//     ASSERT_EQ(tree, std::nullopt);
// }

// TEST(LineTree, MergeDisconnected) {
//     auto tree1 = LineTree({{0, 0}, {10, 0}});
//     auto tree2 = LineTree({{0, 10}, {10, 10}});
//
//     auto tree = tree1.merge(tree2);
//     ASSERT_EQ(tree, std::nullopt);
// }
//
// TEST(LineTree, MergeWithTriangle) {
//     auto tree1 = LineTree({{0, 10}, {10, 10}});
//     auto tree2 = LineTree({{10, 0}, {10, 10}, {20, 10}});
//
//     auto line0 = LineTree::sized_line2d_t {line2d_t {{0, 10}, {10, 10}}, 0, 10};
//     auto line1 = LineTree::sized_line2d_t {line2d_t {{10, 10}, {10, 0}}, 10, 20};
//     auto line2 = LineTree::sized_line2d_t {line2d_t {{10, 10}, {20, 10}}, 10, 20};
//
//     auto tree = tree1.merge(tree2);
//     ASSERT_EQ(tree.has_value(), true);
//
//     EXPECT_EQ(tree->segment_count(), 3);
//     EXPECT_EQ(tree->starts_new_subtree(0), false);
//     EXPECT_EQ(tree->starts_new_subtree(1), false);
//     EXPECT_EQ(tree->starts_new_subtree(2), true);
//
//     EXPECT_THAT(tree->sized_segments(), testing::ElementsAre(line0, line1, line2));
// }
//
// TEST(LineTree, MergeCompletely) {
//     auto tree1 = LineTree({{10, 0}, {20, 0}});
//     auto tree2 = LineTree({{0, 0}, {30, 0}});
//
//     auto line0 = LineTree::sized_line2d_t {line2d_t {{0, 0}, {30, 0}}, 0, 30};
//
//     auto tree_left = tree1.merge(tree2);
//     ASSERT_EQ(tree_left.has_value(), true);
//     EXPECT_THAT(tree_left->sized_segments(), testing::ElementsAre(line0));
//
//     auto tree_right = tree2.merge(tree1);
//     ASSERT_EQ(tree_right.has_value(), true);
//     EXPECT_THAT(tree_right->sized_segments(), testing::ElementsAre(line0));
// }
//
// TEST(LineTree, MergeAndSplit) {
//     auto tree1 = LineTree({{10, 0}, {20, 0}, {20, 10}});
//     auto tree2 = LineTree({{0, 0}, {30, 0}});
//
//     auto line0 = LineTree::sized_line2d_t {line2d_t {{0, 0}, {20, 0}}, 0, 20};
//     auto line1 = LineTree::sized_line2d_t {line2d_t {{20, 0}, {20, 10}}, 20, 30};
//     auto line2 = LineTree::sized_line2d_t {line2d_t {{20, 0}, {30, 0}}, 20, 30};
//
//     auto tree_left = tree1.merge(tree2);
//     ASSERT_EQ(tree_left.has_value(), true);
//     EXPECT_THAT(tree_left->sized_segments(), testing::ElementsAre(line0, line1,
//     line2));
//
//     auto tree_right = tree2.merge(tree1);
//     ASSERT_EQ(tree_right.has_value(), true);
//     EXPECT_THAT(tree_right->sized_segments(), testing::ElementsAre(line0, line1,
//     line2));
// }
//
// TEST(LineTree, MergerSplitInsideLine) {
//     auto tree1 = LineTree({{0, 0}, {20, 0}});
//     auto tree2 = LineTree({{10, 0}, {10, 10}});
//
//     auto line0 = LineTree::sized_line2d_t {line2d_t {{0, 0}, {10, 0}}, 0, 10};
//     auto line1 = LineTree::sized_line2d_t {line2d_t {{10, 0}, {10, 10}}, 10, 20};
//     auto line2 = LineTree::sized_line2d_t {line2d_t {{10, 0}, {20, 0}}, 10, 20};
//
//     auto tree_merged = tree1.merge(tree2);
//     ASSERT_EQ(tree_merged.has_value(), true);
//     EXPECT_THAT(tree_merged->sized_segments(), testing::ElementsAre(line0, line1,
//     line2));
// }
//
// TEST(LineTree, MergeTwoTimes) {
//     auto tree1 = LineTree({{0, 0}, {0, 5}});
//     auto tree2 = LineTree({{0, 1}, {1, 1}});
//     auto tree3 = LineTree({{0, 2}, {2, 2}});
//
//     auto line0 = LineTree::sized_line2d_t {line2d_t {{0, 0}, {0, 1}}, 0, 1};
//     auto line1 = LineTree::sized_line2d_t {line2d_t {{0, 1}, {0, 2}}, 1, 2};
//     auto line2 = LineTree::sized_line2d_t {line2d_t {{0, 2}, {0, 5}}, 2, 5};
//     auto line3 = LineTree::sized_line2d_t {line2d_t {{0, 2}, {2, 2}}, 2, 4};
//     auto line4 = LineTree::sized_line2d_t {line2d_t {{0, 1}, {1, 1}}, 1, 2};
//
//     auto tree_tmp = tree1.merge(tree2);
//     ASSERT_EQ(tree_tmp.has_value(), true);
//     auto tree_merged = tree_tmp->merge(tree3);
//     ASSERT_EQ(tree_merged.has_value(), true);
//
//     EXPECT_THAT(tree_merged->sized_segments(),
//                 testing::ElementsAre(line0, line1, line2, line3, line4));
// }

//
// Reroot
//

TEST(LineTree, RerootSimple) {
    auto tree = LineTree({{0, 0}, {10, 0}});

    auto line0 = LineTree::sized_line2d_t {line2d_t {{10, 0}, {0, 0}}, 0, 10};

    auto tree_reroot = tree.reroot({10, 0});
    EXPECT_THAT(tree_reroot.has_value(), true);
    EXPECT_THAT(tree_reroot->sized_segments(), testing::ElementsAre(line0));
}

TEST(LineTree, RerootSameRoot) {
    auto tree = LineTree({{0, 0}, {10, 0}});

    auto line0 = LineTree::sized_line2d_t {line2d_t {{0, 0}, {10, 0}}, 0, 10};

    auto tree_reroot = tree.reroot({0, 0});
    EXPECT_THAT(tree_reroot.has_value(), true);
    EXPECT_THAT(tree_reroot->sized_segments(), testing::ElementsAre(line0));
}

TEST(LineTree, RerootImpossibleRoot) {
    auto tree = LineTree({{0, 0}, {10, 0}});

    auto line0 = LineTree::sized_line2d_t {line2d_t {{0, 0}, {10, 0}}, 0, 10};

    auto tree_reroot = tree.reroot({10, 10});
    EXPECT_EQ(tree_reroot, std::nullopt);
}

}  // namespace logicsim