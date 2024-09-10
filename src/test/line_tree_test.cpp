
#include "line_tree.h"

#include "logging.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>

namespace logicsim {
/*

TEST(LineTree2, DefaultCreation) {
    const auto tree = LineTree2 {};
    ASSERT_EQ(tree.segment_count(), 0);
}

TEST(LineTree2, ListCreation) {
    const auto tree = LineTree2 {point_t {0, 0}, point_t {10, 0}, point_t {10, 12}};
    ASSERT_EQ(tree.segment_count(), 2);
}

TEST(LineTree2, TestSegments) {
    const auto tree = LineTree2 {point_t {0, 0}, point_t {10, 0}, point_t {10, 12}};

    auto line0 = line_t {point_t {0, 0}, point_t {10, 0}};
    auto line1 = line_t {point_t {10, 0}, point_t {10, 12}};

    ASSERT_EQ(tree.segment_count(), 2);
    ASSERT_EQ(tree.segment(0), line0);
    ASSERT_EQ(tree.segment(1), line1);
}

TEST(LineTree2, TestSegmentIterator) {
    const auto tree = LineTree2 {point_t {0, 0}, point_t {10, 0}, point_t {10, 12}};

    auto line0 = line_t {point_t {0, 0}, point_t {10, 0}};
    auto line1 = line_t {point_t {10, 0}, point_t {10, 12}};

    ASSERT_THAT(tree.segments(), testing::ElementsAre(line0, line1));
}

TEST(LineTree2, TestInternalPointsIteratorEmpty) {
    const auto tree = LineTree2 {};

    ASSERT_THAT(tree.internal_points(), testing::ElementsAre());
}

TEST(LineTree2, TestInternalPointsIteratorTwo) {
    const auto tree = LineTree2 {point_t {0, 0}, point_t {0, 10}};

    ASSERT_THAT(tree.internal_points(), testing::ElementsAre());
}

TEST(LineTree2, TestInternalPointsIteratorThree) {
    const auto tree = LineTree2 {point_t {0, 0}, point_t {10, 0}, point_t {10, 12}};

    ASSERT_THAT(tree.internal_points(), testing::ElementsAre(point_t {10, 0}));
}

TEST(LineTree2, TestInternalPointsIteratorMergedTwo) {
    auto tree1 = LineTree2({point_t {0, 0}, point_t {10, 0}});
    auto tree2 = LineTree2({point_t {5, 0}, point_t {5, 10}});

    auto tree = merge({tree1, tree2});
    ASSERT_EQ(tree.has_value(), true);

    ASSERT_THAT(tree->internal_points(), testing::ElementsAre(point_t {5, 0}));
}

TEST(LineTree2, TestInternalPointsIteratorMergedThree) {
    auto tree1 = LineTree2({point_t {0, 0}, point_t {10, 0}});
    auto tree2 = LineTree2({point_t {5, 0}, point_t {5, 10}});
    auto tree3 = LineTree2({point_t {2, 0}, point_t {2, 20}, point_t {10, 20}});

    auto tree = merge({tree1, tree2, tree3});
    ASSERT_EQ(tree.has_value(), true);

    ASSERT_THAT(tree->internal_points(),
                testing::ElementsAre(point_t {2, 0}, point_t {2, 20}, point_t {5, 0}));
}

TEST(LineTree2, TestSizeSegmentIterator) {
    const auto tree =
        LineTree2 {point_t {0, 0}, point_t {10, 0}, point_t {10, 12}, point_t {20, 12}};

    auto line0 = LineTree2::sized_line_t {
        .line = line_t {point_t {0, 0}, point_t {10, 0}},
        .p0_length = 0,
        .p1_length = 10,
        .has_cross_point_p0 = false,
    };
    auto line1 = LineTree2::sized_line_t {
        .line = line_t {point_t {10, 0}, point_t {10, 12}},
        .p0_length = 10,
        .p1_length = 22,
        .has_cross_point_p0 = false,
    };
    auto line2 = LineTree2::sized_line_t {
        .line = line_t {point_t {10, 12}, point_t {20, 12}},
        .p0_length = 22,
        .p1_length = 32,
        .has_cross_point_p0 = false,
    };

    print(tree.sized_segments());
    print(std::vector {line0, line1, line2});

    ASSERT_THAT(tree.sized_segments(), testing::ElementsAre(line0, line1, line2));
}

TEST(LineTree2, TestIteratorNeighborTest) {
    const auto tree =
        LineTree2 {point_t {0, 0}, point_t {10, 0}, point_t {10, 12}, point_t {20, 12}};

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

TEST(LineTree2, CreateWithDiagonalEdges) {
    EXPECT_THROW(LineTree2({point_t {0, 0}, point_t {5, 5}}), InvalidLineTree2Exception);
    EXPECT_THROW(LineTree2({point_t {0, 0}, point_t {0, 10}, point_t {5, 5}}),
                 InvalidLineTree2Exception);
}

TEST(LineTree2, CreateWithUnecessaryPoints) {
    EXPECT_THROW(LineTree2({point_t {0, 0}, point_t {0, 2}, point_t {0, 4}}),
                 InvalidLineTree2Exception);
    EXPECT_THROW(
        LineTree2({point_t {0, 0}, point_t {0, 2}, point_t {2, 2}, point_t {4, 2}}),
        InvalidLineTree2Exception);
}

TEST(LineTree2, CreateWithDuplicates) {
    EXPECT_THROW(LineTree2({point_t {0, 0}, point_t {0, 10}, point_t {10, 10},
                           point_t {10, 0}, point_t {0, 0}}),
                 InvalidLineTree2Exception);
}

TEST(LineTree2, CreateWithCollisions) {
    EXPECT_THROW(LineTree2({point_t {0, 0}, point_t {0, 10}, point_t {0, 5}}),
                 InvalidLineTree2Exception);
    EXPECT_THROW(LineTree2({point_t {0, 0}, point_t {0, 10}, point_t {0, -5}}),
                 InvalidLineTree2Exception);

    EXPECT_THROW(LineTree2({point_t {0, 0}, point_t {0, 10}, point_t {5, 10},
                           point_t {5, 5}, point_t {0, 5}}),
                 InvalidLineTree2Exception);
    EXPECT_THROW(LineTree2({point_t {0, 0}, point_t {0, 10}, point_t {10, 10},
                           point_t {10, 0}, point_t {-10, 0}}),
                 InvalidLineTree2Exception);
}

TEST(LineTree2, CreateWithZeroLengthLine) {
    EXPECT_THROW(LineTree2({point_t {0, 0}, point_t {0, 0}}), InvalidLineTree2Exception);
    EXPECT_THROW(
        LineTree2({point_t {0, 0}, point_t {0, 10}, point_t {0, 10}, point_t {10, 10}}),
        InvalidLineTree2Exception);
}

//
// From segments
//

TEST(LineTree2, FromSegmentsBugfix) {
    const auto segments = {
        ordered_line_t {point_t {8, 8}, point_t {8, 16}},
        ordered_line_t {point_t {8, 13}, point_t {14, 13}},
        ordered_line_t {point_t {11, 10}, point_t {11, 13}},
        ordered_line_t {point_t {11, 13}, point_t {11, 16}},
    };

    const auto tree = LineTree2::from_segments(segments);

    ASSERT_EQ(tree.has_value(), true);
}

//
// Merge
//

TEST(LineTree2, MergeTreesSimple) {
    auto tree1 = LineTree2({point_t {0, 0}, point_t {0, 10}});
    auto tree2 = LineTree2({point_t {0, 10}, point_t {10, 10}});

    auto line0 = LineTree2::sized_line_t {
        .line = line_t {point_t {0, 0}, point_t {0, 10}},
        .p0_length = 0,
        .p1_length = 10,
        .has_cross_point_p0 = false,
    };
    auto line1 = LineTree2::sized_line_t {
        .line = line_t {point_t {0, 10}, point_t {10, 10}},
        .p0_length = 10,
        .p1_length = 20,
        .has_cross_point_p0 = false,
    };

    auto tree = merge({tree1, tree2});

    ASSERT_EQ(tree.has_value(), true);
    EXPECT_THAT(tree->sized_segments(), testing::ElementsAre(line0, line1));
}

TEST(LineTree2, MergeTreesLongChain) {
    auto tree1 = LineTree2({
        point_t {0, 0},
        point_t {0, 10},
        point_t {10, 10},
        point_t {10, 0},
    });
    auto tree2 = LineTree2({
        point_t {10, 0},
        point_t {20, 0},
        point_t {20, 10},
        point_t {30, 10},
        point_t {30, 0},
    });

    auto line0 = LineTree2::sized_line_t {
        .line = line_t {point_t {0, 0}, point_t {0, 10}},
        .p0_length = 0,
        .p1_length = 10,
        .has_cross_point_p0 = false,
    };
    auto line1 = LineTree2::sized_line_t {
        .line = line_t {point_t {0, 10}, point_t {10, 10}},
        .p0_length = 10,
        .p1_length = 20,
        .has_cross_point_p0 = false,
    };
    auto line2 = LineTree2::sized_line_t {
        .line = line_t {point_t {10, 10}, point_t {10, 0}},
        .p0_length = 20,
        .p1_length = 30,
        .has_cross_point_p0 = false,
    };
    auto line3 = LineTree2::sized_line_t {
        .line = line_t {point_t {10, 0}, point_t {20, 0}},
        .p0_length = 30,
        .p1_length = 40,
        .has_cross_point_p0 = false,
    };
    auto line4 = LineTree2::sized_line_t {
        .line = line_t {point_t {20, 0}, point_t {20, 10}},
        .p0_length = 40,
        .p1_length = 50,
        .has_cross_point_p0 = false,
    };
    auto line5 = LineTree2::sized_line_t {
        .line = line_t {point_t {20, 10}, point_t {30, 10}},
        .p0_length = 50,
        .p1_length = 60,
        .has_cross_point_p0 = false,
    };
    auto line6 = LineTree2::sized_line_t {
        .line = line_t {point_t {30, 10}, point_t {30, 0}},
        .p0_length = 60,
        .p1_length = 70,
        .has_cross_point_p0 = false,
    };

    auto tree = merge({tree1, tree2});

    ASSERT_EQ(tree.has_value(), true);
    EXPECT_THAT(tree->sized_segments(),
                testing::ElementsAre(line0, line1, line2, line3, line4, line5, line6));
}

TEST(LineTree2, MergeTreesLongChainInverter) {
    auto tree1 =
        LineTree2({point_t {0, 0}, point_t {0, 10}, point_t {10, 10}, point_t {10, 0}});
    auto tree2 = LineTree2({point_t {10, 0}, point_t {20, 0}, point_t {20, 10},
                           point_t {30, 10}, point_t {30, 0}});

    auto line0 = LineTree2::sized_line_t {
        .line = line_t {point_t {30, 0}, point_t {30, 10}},
        .p0_length = 0,
        .p1_length = 10,
        .has_cross_point_p0 = false,
    };
    auto line1 = LineTree2::sized_line_t {
        .line = line_t {point_t {30, 10}, point_t {20, 10}},
        .p0_length = 10,
        .p1_length = 20,
        .has_cross_point_p0 = false,
    };
    auto line2 = LineTree2::sized_line_t {
        .line = line_t {point_t {20, 10}, point_t {20, 0}},
        .p0_length = 20,
        .p1_length = 30,
        .has_cross_point_p0 = false,
    };
    auto line3 = LineTree2::sized_line_t {
        .line = line_t {point_t {20, 0}, point_t {10, 0}},
        .p0_length = 30,
        .p1_length = 40,
        .has_cross_point_p0 = false,
    };
    auto line4 = LineTree2::sized_line_t {
        .line = line_t {point_t {10, 0}, point_t {10, 10}},
        .p0_length = 40,
        .p1_length = 50,
        .has_cross_point_p0 = false,
    };
    auto line5 = LineTree2::sized_line_t {
        .line = line_t {point_t {10, 10}, point_t {0, 10}},
        .p0_length = 50,
        .p1_length = 60,
        .has_cross_point_p0 = false,
    };
    auto line6 = LineTree2::sized_line_t {
        .line = line_t {point_t {0, 10}, point_t {0, 0}},
        .p0_length = 60,
        .p1_length = 70,
        .has_cross_point_p0 = false,
    };

    auto tree = merge({tree1, tree2}, point_t {30, 0});

    ASSERT_EQ(tree.has_value(), true);
    EXPECT_THAT(tree->sized_segments(),
                testing::ElementsAre(line0, line1, line2, line3, line4, line5, line6));
}

TEST(LineTree2, MergeNoRoot) {
    auto tree1 = LineTree2({point_t {0, 0}, point_t {0, 10}, point_t {10, 10}});
    auto tree2 = LineTree2({point_t {0, 0}, point_t {10, 0}, point_t {10, 10}});

    auto tree = merge({tree1, tree2});
    ASSERT_EQ(tree, std::nullopt);
}

TEST(LineTree2, MergeWithLoop) {
    auto tree1 =
        LineTree2({point_t {0, 0}, point_t {0, 10}, point_t {10, 10}, point_t {10, 0}});
    auto tree2 =
        LineTree2({point_t {10, 0}, point_t {20, 0}, point_t {20, 10}, point_t {10, 10}});

    auto tree = merge({tree1, tree2});
    ASSERT_EQ(tree, std::nullopt);
}

TEST(LineTree2, MergeTwoSidesLoop) {
    auto tree1 = LineTree2({point_t {1, 0}, point_t {2, 0}, point_t {2, 1}, point_t {3,
1}, point_t {3, 0}, point_t {4, 0}}); auto tree2 = LineTree2({point_t {0, 0}, point_t {4,
0}});

    auto tree = merge({tree1, tree2});
    ASSERT_EQ(tree, std::nullopt);
}

TEST(LineTree2, MergeDisconnected) {
    auto tree1 = LineTree2({point_t {0, 0}, point_t {10, 0}});
    auto tree2 = LineTree2({point_t {0, 10}, point_t {10, 10}});

    auto tree = merge({tree1, tree2});
    ASSERT_EQ(tree, std::nullopt);
}

TEST(LineTree2, MergeWithTriangle) {
    auto tree1 = LineTree2({point_t {0, 10}, point_t {10, 10}});
    auto tree2 = LineTree2({point_t {10, 0}, point_t {10, 10}, point_t {20, 10}});

    auto line0 = LineTree2::sized_line_t {
        .line = line_t {point_t {0, 10}, point_t {10, 10}},
        .p0_length = 0,
        .p1_length = 10,
        .has_cross_point_p0 = false,
    };
    auto line1 = LineTree2::sized_line_t {
        .line = line_t {point_t {10, 10}, point_t {10, 0}},
        .p0_length = 10,
        .p1_length = 20,
        .has_cross_point_p0 = false,
    };
    auto line2 = LineTree2::sized_line_t {
        .line = line_t {point_t {10, 10}, point_t {20, 10}},
        .p0_length = 10,
        .p1_length = 20,
        .has_cross_point_p0 = true,
    };

    auto tree = merge({tree1, tree2});
    ASSERT_EQ(tree.has_value(), true);

    EXPECT_EQ(tree->segment_count(), 3);

    EXPECT_THAT(tree->sized_segments(), testing::ElementsAre(line0, line1, line2));
}

TEST(LineTree2, MergeCompleteOveralapp) {
    auto tree1 = LineTree2({point_t {10, 0}, point_t {20, 0}});
    auto tree2 = LineTree2({point_t {0, 0}, point_t {30, 0}});

    auto line0 = LineTree2::sized_line_t {
        .line = line_t {point_t {0, 0}, point_t {30, 0}},
        .p0_length = 0,
        .p1_length = 30,
        .has_cross_point_p0 = false,
    };

    auto tree_left = merge({tree1, tree2});
    ASSERT_EQ(tree_left.has_value(), true);
    EXPECT_THAT(tree_left->sized_segments(), testing::ElementsAre(line0));

    auto tree_right = merge({tree2, tree1});
    ASSERT_EQ(tree_right.has_value(), true);
    EXPECT_THAT(tree_right->sized_segments(), testing::ElementsAre(line0));
}

TEST(LineTree2, MergeAndSplit) {
    auto tree1 = LineTree2({point_t {10, 0}, point_t {20, 0}, point_t {20, 10}});
    auto tree2 = LineTree2({point_t {0, 0}, point_t {30, 0}});

    auto line0 = LineTree2::sized_line_t {
        .line = line_t {point_t {0, 0}, point_t {20, 0}},
        .p0_length = 0,
        .p1_length = 20,
        .has_cross_point_p0 = false,
    };
    auto line1 = LineTree2::sized_line_t {
        .line = line_t {point_t {20, 0}, point_t {20, 10}},
        .p0_length = 20,
        .p1_length = 30,
        .has_cross_point_p0 = false,
    };
    auto line2 = LineTree2::sized_line_t {
        .line = line_t {point_t {20, 0}, point_t {30, 0}},
        .p0_length = 20,
        .p1_length = 30,
        .has_cross_point_p0 = true,
    };

    auto tree_left = merge({tree1, tree2});
    ASSERT_EQ(tree_left.has_value(), true);
    EXPECT_THAT(tree_left->sized_segments(), testing::ElementsAre(line0, line1, line2));

    auto tree_right = merge({tree2, tree1});
    ASSERT_EQ(tree_right.has_value(), true);
    EXPECT_THAT(tree_right->sized_segments(), testing::ElementsAre(line0, line1, line2));
}

TEST(LineTree2, MergerSplitInsideLine) {
    auto tree1 = LineTree2({point_t {0, 0}, point_t {20, 0}});
    auto tree2 = LineTree2({point_t {10, 0}, point_t {10, 10}});

    auto line0 = LineTree2::sized_line_t {
        .line = line_t {point_t {0, 0}, point_t {10, 0}},
        .p0_length = 0,
        .p1_length = 10,
        .has_cross_point_p0 = false,
    };
    auto line1 = LineTree2::sized_line_t {
        .line = line_t {point_t {10, 0}, point_t {10, 10}},
        .p0_length = 10,
        .p1_length = 20,
        .has_cross_point_p0 = false,
    };
    auto line2 = LineTree2::sized_line_t {
        .line = line_t {point_t {10, 0}, point_t {20, 0}},
        .p0_length = 10,
        .p1_length = 20,
        .has_cross_point_p0 = true,
    };

    auto tree_merged = merge({tree1, tree2});
    ASSERT_EQ(tree_merged.has_value(), true);
    EXPECT_THAT(tree_merged->sized_segments(), testing::ElementsAre(line0, line1, line2));
}

TEST(LineTree2, MergeThreeTrees) {
    auto tree1 = LineTree2({point_t {0, 0}, point_t {0, 5}});
    auto tree2 = LineTree2({point_t {0, 1}, point_t {1, 1}});
    auto tree3 = LineTree2({point_t {0, 2}, point_t {2, 2}});

    auto line0 = LineTree2::sized_line_t {
        .line = line_t {point_t {0, 0}, point_t {0, 1}},
        .p0_length = 0,
        .p1_length = 1,
        .has_cross_point_p0 = false,
    };
    auto line1 = LineTree2::sized_line_t {
        .line = line_t {point_t {0, 1}, point_t {0, 2}},
        .p0_length = 1,
        .p1_length = 2,
        .has_cross_point_p0 = false,
    };
    auto line2 = LineTree2::sized_line_t {
        .line = line_t {point_t {0, 2}, point_t {0, 5}},
        .p0_length = 2,
        .p1_length = 5,
        .has_cross_point_p0 = false,
    };
    auto line3 = LineTree2::sized_line_t {
        .line = line_t {point_t {0, 2}, point_t {2, 2}},
        .p0_length = 2,
        .p1_length = 4,
        .has_cross_point_p0 = true,
    };
    auto line4 = LineTree2::sized_line_t {
        .line = line_t {point_t {0, 1}, point_t {1, 1}},
        .p0_length = 1,
        .p1_length = 2,
        .has_cross_point_p0 = true,
    };

    auto tree_merged = merge({tree1, tree2, tree3});
    ASSERT_EQ(tree_merged.has_value(), true);

    EXPECT_THAT(tree_merged->sized_segments(),
                testing::ElementsAre(line0, line1, line2, line3, line4));
}

//
// output count
//

TEST(LineTree2, OutputCoundAndDelays) {
    auto tree1 = LineTree2({point_t {0, 0}, point_t {0, 5}});
    auto tree2 = LineTree2({point_t {0, 1}, point_t {1, 1}});
    auto tree3 = LineTree2({point_t {0, 2}, point_t {2, 2}});

    auto tree_merged = merge({tree1, tree2, tree3});
    ASSERT_EQ(tree_merged.has_value(), true);

    EXPECT_THAT(tree1.output_count(), connection_count_t {1});
    EXPECT_THAT(tree2.output_count(), connection_count_t {1});
    EXPECT_THAT(tree3.output_count(), connection_count_t {1});
    EXPECT_THAT(tree_merged->output_count(), connection_count_t {3});

    EXPECT_THAT(tree1.calculate_output_lengths(), testing::ElementsAre(5));
    EXPECT_THAT(tree2.calculate_output_lengths(), testing::ElementsAre(1));
    EXPECT_THAT(tree3.calculate_output_lengths(), testing::ElementsAre(2));
    EXPECT_THAT(tree_merged->calculate_output_lengths(), testing::ElementsAre(5, 4, 2));
}

//
// Output Positions
//

TEST(LineTree2, OutputPositions) {
    auto tree1 = LineTree2({point_t {0, 0}, point_t {0, 5}});
    auto tree2 = LineTree2({point_t {0, 1}, point_t {1, 1}});
    auto tree3 = LineTree2({point_t {0, 2}, point_t {2, 2}});

    auto tree_merged = merge({tree1, tree2, tree3});
    ASSERT_EQ(tree_merged.has_value(), true);

    EXPECT_THAT(tree1.output_positions().size(), 1);
    EXPECT_THAT(tree2.output_positions().size(), 1);
    EXPECT_THAT(tree3.output_positions().size(), 1);
    EXPECT_THAT(tree_merged->output_positions().size(), 3);

    EXPECT_THAT(tree1.output_positions(), testing::ElementsAre(point_t {0, 5}));
    EXPECT_THAT(tree2.output_positions(), testing::ElementsAre(point_t {1, 1}));
    EXPECT_THAT(tree3.output_positions(), testing::ElementsAre(point_t {2, 2}));
    EXPECT_THAT(tree_merged->output_positions(),
                testing::ElementsAre(point_t {0, 5}, point_t {2, 2}, point_t {1, 1}));
}

//
// Reroot
//

TEST(LineTree2, RerootSimple) {
    auto tree = LineTree2({point_t {0, 0}, point_t {10, 0}});

    auto line0 = LineTree2::sized_line_t {
        .line = line_t {point_t {10, 0}, point_t {0, 0}},
        .p0_length = 0,
        .p1_length = 10,
        .has_cross_point_p0 = false,
    };

    auto tree_reroot = tree.reroot(point_t {10, 0});
    ASSERT_EQ(tree_reroot.has_value(), true);
    EXPECT_THAT(tree_reroot->sized_segments(), testing::ElementsAre(line0));
}

TEST(LineTree2, RerootSameRoot) {
    auto tree = LineTree2({point_t {0, 0}, point_t {10, 0}});

    auto line0 = LineTree2::sized_line_t {
        .line = line_t {point_t {0, 0}, point_t {10, 0}},
        .p0_length = 0,
        .p1_length = 10,
        .has_cross_point_p0 = false,
    };

    auto tree_reroot = tree.reroot(point_t {0, 0});
    ASSERT_EQ(tree_reroot.has_value(), true);
    EXPECT_THAT(tree_reroot->sized_segments(), testing::ElementsAre(line0));
}

TEST(LineTree2, RerootImpossibleRoot) {
    auto tree = LineTree2({point_t {0, 0}, point_t {10, 0}});

    auto tree_reroot = tree.reroot(point_t {10, 10});
    ASSERT_EQ(tree_reroot, std::nullopt);
}
*/

}  // namespace logicsim
