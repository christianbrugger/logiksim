
#include "collision.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

TEST(Collision, TestPointInLineHorizontal) {
    ASSERT_EQ(is_colliding(point_t {0, 0}, line_t {{0, 0}, {0, 2}}), true);
    ASSERT_EQ(is_colliding(point_t {0, 1}, line_t {{0, 0}, {0, 2}}), true);
    ASSERT_EQ(is_colliding(point_t {0, 2}, line_t {{0, 0}, {0, 2}}), true);

    ASSERT_EQ(is_colliding(point_t {1, 0}, line_t {{0, 0}, {0, 2}}), false);
    ASSERT_EQ(is_colliding(point_t {0, -1}, line_t {{0, 0}, {0, 2}}), false);
    ASSERT_EQ(is_colliding(point_t {0, 3}, line_t {{0, 0}, {0, 2}}), false);
}

TEST(Collision, TestPointInLineHorizontalFlipped) {
    ASSERT_EQ(is_colliding(point_t {0, 0}, line_t {{0, 2}, {0, 0}}), true);
    ASSERT_EQ(is_colliding(point_t {0, 1}, line_t {{0, 2}, {0, 0}}), true);
    ASSERT_EQ(is_colliding(point_t {0, 2}, line_t {{0, 2}, {0, 0}}), true);

    ASSERT_EQ(is_colliding(point_t {1, 0}, line_t {{0, 2}, {0, 0}}), false);
    ASSERT_EQ(is_colliding(point_t {0, -1}, line_t {{0, 2}, {0, 0}}), false);
    ASSERT_EQ(is_colliding(point_t {0, 3}, line_t {{0, 2}, {0, 0}}), false);
}

TEST(Collision, TestPointInLineVertical) {
    ASSERT_EQ(is_colliding(point_t {0, 0}, line_t {{0, 0}, {2, 0}}), true);
    ASSERT_EQ(is_colliding(point_t {1, 0}, line_t {{0, 0}, {2, 0}}), true);
    ASSERT_EQ(is_colliding(point_t {2, 0}, line_t {{0, 0}, {2, 0}}), true);

    ASSERT_EQ(is_colliding(point_t {0, 1}, line_t {{0, 0}, {2, 0}}), false);
    ASSERT_EQ(is_colliding(point_t {-1, 0}, line_t {{0, 0}, {2, 0}}), false);
    ASSERT_EQ(is_colliding(point_t {3, 0}, line_t {{0, 0}, {2, 0}}), false);
}

TEST(Collision, TestPointInLineVerticalFlipped) {
    ASSERT_EQ(is_colliding(point_t {0, 0}, line_t {{2, 0}, {0, 0}}), true);
    ASSERT_EQ(is_colliding(point_t {1, 0}, line_t {{2, 0}, {0, 0}}), true);
    ASSERT_EQ(is_colliding(point_t {2, 0}, line_t {{2, 0}, {0, 0}}), true);

    ASSERT_EQ(is_colliding(point_t {0, 1}, line_t {{2, 0}, {0, 0}}), false);
    ASSERT_EQ(is_colliding(point_t {-1, 0}, line_t {{2, 0}, {0, 0}}), false);
    ASSERT_EQ(is_colliding(point_t {3, 0}, line_t {{2, 0}, {0, 0}}), false);
}

}  // namespace logicsim