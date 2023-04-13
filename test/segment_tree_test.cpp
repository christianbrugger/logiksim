
#include "segment_tree.h"

#include "format.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

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
}  // namespace logicsim