
#include "collision.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

TEST(Collision, TestPointInLineHorizontal) {
    ASSERT_EQ(point_in_line(point2d_t {0, 0}, line2d_t {{0, 0}, {0, 2}}), true);
    ASSERT_EQ(point_in_line(point2d_t {0, 1}, line2d_t {{0, 0}, {0, 2}}), true);
    ASSERT_EQ(point_in_line(point2d_t {0, 2}, line2d_t {{0, 0}, {0, 2}}), true);

    ASSERT_EQ(point_in_line(point2d_t {1, 0}, line2d_t {{0, 0}, {0, 2}}), false);
    ASSERT_EQ(point_in_line(point2d_t {0, -1}, line2d_t {{0, 0}, {0, 2}}), false);
    ASSERT_EQ(point_in_line(point2d_t {0, 3}, line2d_t {{0, 0}, {0, 2}}), false);
}

TEST(Collision, TestPointInLineHorizontalFlipped) {
    ASSERT_EQ(point_in_line(point2d_t {0, 0}, line2d_t {{0, 2}, {0, 0}}), true);
    ASSERT_EQ(point_in_line(point2d_t {0, 1}, line2d_t {{0, 2}, {0, 0}}), true);
    ASSERT_EQ(point_in_line(point2d_t {0, 2}, line2d_t {{0, 2}, {0, 0}}), true);

    ASSERT_EQ(point_in_line(point2d_t {1, 0}, line2d_t {{0, 2}, {0, 0}}), false);
    ASSERT_EQ(point_in_line(point2d_t {0, -1}, line2d_t {{0, 2}, {0, 0}}), false);
    ASSERT_EQ(point_in_line(point2d_t {0, 3}, line2d_t {{0, 2}, {0, 0}}), false);
}

TEST(Collision, TestPointInLineVertical) {
    ASSERT_EQ(point_in_line(point2d_t {0, 0}, line2d_t {{0, 0}, {2, 0}}), true);
    ASSERT_EQ(point_in_line(point2d_t {1, 0}, line2d_t {{0, 0}, {2, 0}}), true);
    ASSERT_EQ(point_in_line(point2d_t {2, 0}, line2d_t {{0, 0}, {2, 0}}), true);

    ASSERT_EQ(point_in_line(point2d_t {0, 1}, line2d_t {{0, 0}, {2, 0}}), false);
    ASSERT_EQ(point_in_line(point2d_t {-1, 0}, line2d_t {{0, 0}, {2, 0}}), false);
    ASSERT_EQ(point_in_line(point2d_t {3, 0}, line2d_t {{0, 0}, {2, 0}}), false);
}

TEST(Collision, TestPointInLineVerticalFlipped) {
    ASSERT_EQ(point_in_line(point2d_t {0, 0}, line2d_t {{2, 0}, {0, 0}}), true);
    ASSERT_EQ(point_in_line(point2d_t {1, 0}, line2d_t {{2, 0}, {0, 0}}), true);
    ASSERT_EQ(point_in_line(point2d_t {2, 0}, line2d_t {{2, 0}, {0, 0}}), true);

    ASSERT_EQ(point_in_line(point2d_t {0, 1}, line2d_t {{2, 0}, {0, 0}}), false);
    ASSERT_EQ(point_in_line(point2d_t {-1, 0}, line2d_t {{2, 0}, {0, 0}}), false);
    ASSERT_EQ(point_in_line(point2d_t {3, 0}, line2d_t {{2, 0}, {0, 0}}), false);
}

}  // namespace logicsim