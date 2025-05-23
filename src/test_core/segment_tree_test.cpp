
#include "core/segment_tree.h"

#include "core/algorithm/range.h"
#include "core/algorithm/uniform_int_distribution.h"
#include "core/logging.h"
#include "core/random/bool.h"
#include "core/random/generator.h"
#include "core/random/part.h"
#include "core/random/segment.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

TEST(SegmentTree, NormalizeSegmentOrder) {
    auto tree = SegmentTree {};

    const auto info0 = segment_info_t {
        .line = ordered_line_t {point_t {0, 0}, point_t {5, 0}},
        .p0_type = SegmentPointType::corner_point,
        .p1_type = SegmentPointType::corner_point,
    };
    const auto info1 = segment_info_t {
        .line = ordered_line_t {point_t {1, 0}, point_t {6, 0}},
        .p0_type = SegmentPointType::shadow_point,
        .p1_type = SegmentPointType::new_unknown,
    };
    const auto info2 = segment_info_t {
        .line = ordered_line_t {point_t {2, 0}, point_t {7, 0}},
        .p0_type = SegmentPointType::output,
        .p1_type = SegmentPointType::output,
    };

    tree.add_segment(info1);
    tree.add_segment(info0);
    tree.add_segment(info2);

    tree.normalize();

    EXPECT_EQ(tree.info(segment_index_t {0}), info0);
    EXPECT_EQ(tree.info(segment_index_t {1}), info1);
    EXPECT_EQ(tree.info(segment_index_t {2}), info2);
}

TEST(SegmentTree, NormalizePointTypeOrder) {
    auto tree = SegmentTree {};

    const auto info0 = segment_info_t {
        .line = ordered_line_t {point_t {0, 0}, point_t {5, 0}},
        .p0_type = SegmentPointType::cross_point,
        .p1_type = static_cast<SegmentPointType>(1),
    };
    const auto info1 = segment_info_t {
        .line = ordered_line_t {point_t {1, 0}, point_t {5, 0}},
        .p0_type = SegmentPointType::shadow_point,
        .p1_type = static_cast<SegmentPointType>(0),
    };
    const auto info2 = segment_info_t {
        .line = ordered_line_t {point_t {2, 0}, point_t {5, 0}},
        .p0_type = SegmentPointType::shadow_point,
        .p1_type = static_cast<SegmentPointType>(2),
    };

    tree.add_segment(info0);
    tree.add_segment(info1);
    tree.add_segment(info2);

    tree.normalize();

    // same
    EXPECT_EQ(tree.info(segment_index_t {0}).line, info0.line);
    EXPECT_EQ(tree.info(segment_index_t {1}).line, info1.line);
    EXPECT_EQ(tree.info(segment_index_t {2}).line, info2.line);

    // same
    EXPECT_EQ(tree.info(segment_index_t {0}).p0_type, info0.p0_type);
    EXPECT_EQ(tree.info(segment_index_t {1}).p0_type, info1.p0_type);
    EXPECT_EQ(tree.info(segment_index_t {2}).p0_type, info2.p0_type);

    // changed
    EXPECT_EQ(tree.info(segment_index_t {0}).p1_type, info1.p1_type);
    EXPECT_EQ(tree.info(segment_index_t {1}).p1_type, info0.p1_type);
    EXPECT_EQ(tree.info(segment_index_t {2}).p1_type, info2.p1_type);
}

//
// Random Tests
//

auto prepare_tree_eq(SegmentTree tree1, SegmentTree tree2, bool expected_equal = true) {
    tree1.normalize();
    tree2.normalize();

    if ((tree1 == tree2) != expected_equal) {
        print();
        print("Tree 1:");
        print(tree1);
        print();
        print("Tree 2:");
        print(tree2);
        print();
    }
    return std::make_pair(tree1, tree2);
}

auto add_n_random_segments(Rng& rng, SegmentTree& tree, unsigned int min = 0,
                           unsigned int max = 10) -> void {
    const auto n = uint_distribution(min, max)(rng);

    for (auto _ [[maybe_unused]] : range(n)) {
        add_random_segment(rng, tree);
    }
}

auto get_random_index(Rng& rng, const SegmentTree& tree) -> segment_index_t {
    return segment_index_t {
        uint_distribution(tree.first_index().value, tree.last_index().value)(rng)};
}

auto add_copy_remove(Rng& rng, SegmentTree& tree) -> void {
    const auto index = get_random_index(rng, tree);

    const auto new_index = tree.copy_segment(tree, index);
    if (tree.info(new_index) != tree.info(index)) {
        throw std::runtime_error("assertion failed");
    }

    tree.swap_and_delete_segment(index);
}

auto copy_shrink_merge(Rng& rng, SegmentTree& tree) -> void {
    const auto index0 = get_random_index(rng, tree);
    const auto full_part = to_part(tree.line(index0));
    auto part0 = get_random_part(rng, tree.line(index0));

    if (get_random_bool(rng)) {
        if (get_random_bool(rng)) {
            part0.begin = full_part.begin;
        } else {
            part0.end = full_part.end;
        }
    }

    if (part0 == full_part) {
        return;
    }
    if (a_inside_b_touching_one_side(part0, full_part)) {
        const auto part1 = difference_touching_one_side(full_part, part0);

        const auto index1 = tree.copy_segment(tree, index0, part1);
        tree.shrink_segment(index0, part0);
        tree.swap_and_merge_segment({.index_merge_to = index0, .index_deleted = index1});
    }

    else {
        const auto [part1, part2] = difference_not_touching(full_part, part0);

        const auto index1 = tree.copy_segment(tree, index0, part1);
        const auto index2 = tree.copy_segment(tree, index0, part2);

        tree.shrink_segment(index0, part0);

        if (get_random_bool(rng)) {
            tree.swap_and_merge_segment(
                {.index_merge_to = index0, .index_deleted = index2});
            tree.swap_and_merge_segment(
                {.index_merge_to = index0, .index_deleted = index1});
        } else {
            tree.swap_and_merge_segment(
                {.index_merge_to = index0, .index_deleted = index1});
            tree.swap_and_merge_segment(
                {.index_merge_to = index0, .index_deleted = index1});
        }
    }
}

TEST(SegmentTree, AddCopyRemove) {
    for (auto i : range(100u)) {
        auto rng = Rng {i};

        // make big tree
        auto tree = SegmentTree {};
        add_n_random_segments(rng, tree, 1);
        const auto tree_orig = SegmentTree {tree};

        // run test
        add_copy_remove(rng, tree);

        // compare
        const auto [tree1, tree2] = prepare_tree_eq(tree, tree_orig);
        ASSERT_EQ(tree1, tree2);
    }
}

TEST(SegmentTree, CopyShrinkMerge) {
    for (auto i : range(100u)) {
        auto rng = Rng {i};

        // make big tree
        auto tree = SegmentTree {};
        add_n_random_segments(rng, tree, 1);
        const auto tree_orig = SegmentTree {tree};

        // run test
        copy_shrink_merge(rng, tree);

        // compare
        const auto [tree1, tree2] = prepare_tree_eq(tree, tree_orig);
        ASSERT_EQ(tree1, tree2);
    }
}

TEST(SegmentTree, MergeTree) {
    for (auto i : range(100u)) {
        auto rng1 = Rng {i};
        auto rng2 = Rng {i};

        // make big tree
        auto tree1 = SegmentTree {};
        add_n_random_segments(rng1, tree1);
        auto tree2 = SegmentTree {};
        add_n_random_segments(rng1, tree2);

        auto tree_m1 = SegmentTree {};
        add_n_random_segments(rng2, tree_m1);
        add_n_random_segments(rng2, tree_m1);

        const auto expected_index = tree1.empty() ? 0 : tree1.last_index().value + 1;

        const auto index = tree1.add_tree(tree2);
        ASSERT_EQ(index.value, expected_index);

        // compare
        const auto [tree_r1, tree_r2] = prepare_tree_eq(tree1, tree_m1);
        ASSERT_EQ(tree_r1, tree_r2);
    }
}

TEST(SegmentTree, MarkInvalid) {
    for (auto i : range(100u)) {
        auto rng = Rng {i};

        auto tree = SegmentTree {};
        const auto index = add_random_segment(rng, tree);

        auto part = get_random_part(rng, tree.info(index).line);
        tree.unmark_valid(index, part);
        const auto tree_1 = SegmentTree {tree};
        tree.unmark_valid(index, part);
        const auto tree_2 = SegmentTree {tree};
        tree.mark_valid(index, part);
        const auto tree_3 = SegmentTree {tree};
        tree.mark_valid(index, part);
        const auto tree_4 = SegmentTree {tree};

        // compare
        {
            const auto [tree_r1, tree_r2] = prepare_tree_eq(tree_1, tree_2);
            ASSERT_EQ(tree_r1, tree_r2);
        }
        {
            const auto [tree_r2, tree_r3] = prepare_tree_eq(tree_2, tree_3, false);
            ASSERT_NE(tree_r2, tree_r3);
        }
        {
            const auto [tree_r3, tree_r4] = prepare_tree_eq(tree_3, tree_4);
            ASSERT_EQ(tree_r3, tree_r4);
        }
    }
}

}  // namespace logicsim