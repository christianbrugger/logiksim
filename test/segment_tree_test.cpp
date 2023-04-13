
#include "segment_tree.h"

#include "format.h"
#include "geometry.h"
#include "range.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

namespace logicsim {

TEST(SegmentTree, NormalizeSegmentOrder) {
    auto tree = SegmentTree {};

    const auto info0 = segment_info_t {
        .line = ordered_line_t {point_t {0, 0}, point_t {5, 0}},
        .p0_type = SegmentPointType::colliding_point,
        .p1_type = SegmentPointType::colliding_point,
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

    EXPECT_EQ(tree.segment_info(segment_index_t {0}), info0);
    EXPECT_EQ(tree.segment_info(segment_index_t {1}), info1);
    EXPECT_EQ(tree.segment_info(segment_index_t {2}), info2);
}

TEST(SegmentTree, NormalizePointTypeOrder) {
    auto tree = SegmentTree {};

    const auto info0 = segment_info_t {
        .line = ordered_line_t {point_t {0, 0}, point_t {5, 0}},
        .p0_type = SegmentPointType::colliding_point,
        .p1_type = static_cast<SegmentPointType>(1),
    };
    const auto info1 = segment_info_t {
        .line = ordered_line_t {point_t {1, 0}, point_t {5, 0}},
        .p0_type = SegmentPointType::shadow_point,
        .p1_type = static_cast<SegmentPointType>(0),
    };
    const auto info2 = segment_info_t {
        .line = ordered_line_t {point_t {2, 0}, point_t {5, 0}},
        .p0_type = SegmentPointType::output,
        .p1_type = static_cast<SegmentPointType>(2),
    };

    tree.add_segment(info0);
    tree.add_segment(info1);
    tree.add_segment(info2);

    tree.normalize();

    // same
    EXPECT_EQ(tree.segment_info(segment_index_t {0}).line, info0.line);
    EXPECT_EQ(tree.segment_info(segment_index_t {1}).line, info1.line);
    EXPECT_EQ(tree.segment_info(segment_index_t {2}).line, info2.line);

    // same
    EXPECT_EQ(tree.segment_info(segment_index_t {0}).p0_type, info0.p0_type);
    EXPECT_EQ(tree.segment_info(segment_index_t {1}).p0_type, info1.p0_type);
    EXPECT_EQ(tree.segment_info(segment_index_t {2}).p0_type, info2.p0_type);

    // changed
    EXPECT_EQ(tree.segment_info(segment_index_t {0}).p1_type, info1.p1_type);
    EXPECT_EQ(tree.segment_info(segment_index_t {1}).p1_type, info0.p1_type);
    EXPECT_EQ(tree.segment_info(segment_index_t {2}).p1_type, info2.p1_type);
}

using Rng = boost::random::mt19937_64;

template <typename T>
using uint_dist = boost::random::uniform_int_distribution<T>;

auto random_part(Rng& rng, ordered_line_t line) -> part_t {
    const auto full_part = to_part(line);

    auto begin = offset_t::value_type {};
    auto end = offset_t::value_type {};
    const auto part_dist = uint_dist {full_part.begin.value, full_part.end.value};
    while (begin >= end) {
        begin = part_dist(rng);
        end = part_dist(rng);
    }
    return part_t {offset_t {begin}, offset_t {end}};
}

auto add_random_segment(Rng& rng, SegmentTree& tree) -> void {
    const auto grid = uint_dist {grid_t::min(), grid_t::max()};

    const auto point_type = [&]() {
        return grid(rng) > 0 ? SegmentPointType::shadow_point : SegmentPointType::output;
    };

    auto p0 = point_t {grid(rng), grid(rng)};
    auto p1 = point_t {grid(rng), grid(rng)};

    if (grid(rng) > 0) {
        p0.x = p1.x;
    } else {
        p0.y = p1.y;
    }

    if (p0 == p1) {
        add_random_segment(rng, tree);
    }

    const auto line = ordered_line_t {line_t {p0, p1}};

    const auto info = segment_info_t {
        .line = line,
        .p0_type = point_type(),
        .p1_type = point_type(),
    };

    auto orignal_count = tree.segment_count();
    const auto new_index = tree.add_segment(info);

    // invariant
    ASSERT_EQ(tree.segment_count(), orignal_count + 1);
    ASSERT_EQ(tree.segment_info(new_index), info);

    const auto part = random_part(rng, line);

    tree.mark_valid(new_index, part);
}

auto validate_tree_eq(SegmentTree tree1, SegmentTree tree2) {
    tree1.normalize();
    tree2.normalize();
    if (tree1 != tree2) {
        print();
        print("Tree 1:");
        print(tree1);
        print();
        print("Tree 2:");
        print(tree2);
        print();
    }
    ASSERT_EQ(tree1, tree2);
}

auto add_n_random_segments(Rng& rng, SegmentTree& tree, unsigned int min = 0,
                           unsigned int max = 100) -> void {
    const auto n = uint_dist {min, max}(rng);

    for (auto _ [[maybe_unused]] : range(n + 1)) {
        add_random_segment(rng, tree);
    }
}

auto get_random_index(Rng& rng, const SegmentTree& tree) -> segment_index_t {
    return segment_index_t {
        uint_dist {tree.first_index().value, tree.last_index().value}(rng)};
}

auto add_copy_remove(Rng& rng, SegmentTree& tree) -> void {
    const auto orig_tree = SegmentTree {tree};

    const auto index = get_random_index(rng, tree);

    const auto new_index = tree.copy_segment(tree, index);
    ASSERT_EQ(tree.segment_info(new_index), tree.segment_info(index));

    tree.swap_and_delete_segment(index);
    validate_tree_eq(orig_tree, tree);
}

auto copy_shrink_merge(Rng& rng, SegmentTree& tree) -> void {
    const auto orig_tree = SegmentTree {tree};

    const auto index = get_random_index(rng, tree);

    // TODO finish test
}

TEST(SegmentTree, AddCopyRemove) {
    for (auto i : range(2u)) {
        auto rng = Rng {i};

        auto tree = SegmentTree {};
        add_n_random_segments(rng, tree, 1);
        add_copy_remove(rng, tree);
    }
}

}  // namespace logicsim