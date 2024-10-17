#include "vocabulary/point.h"

#include "logging.h"

#include <gtest/gtest.h>

#include <ankerl/unordered_dense.h>

namespace logicsim {

TEST(VocabularyPoint, Overflow) {
    // add
    EXPECT_EQ(point_t(10, 20) + point_t(1, 2), point_t(11, 22));
    {
        auto offset = point_t {10, 20};
        offset += point_t {1, 2};
        EXPECT_EQ(offset, point_t(11, 22));
    }
    EXPECT_THROW(static_cast<void>(point_t(grid_t::max(), 0) + point_t(1, 1)),
                 std::exception);
    EXPECT_THROW(static_cast<void>(point_t(0, grid_t::max()) + point_t(1, 1)),
                 std::exception);

    // substract
    EXPECT_EQ(point_t(10, 20) - point_t(1, 2), point_t(9, 18));
    {
        auto offset = point_t {10, 20};
        offset -= point_t {1, 2};
        EXPECT_EQ(offset, point_t(9, 18));
    }
    EXPECT_THROW(static_cast<void>(point_t(grid_t::min(), 0) - point_t(1, 1)),
                 std::exception);
    EXPECT_THROW(static_cast<void>(point_t(0, grid_t::min()) - point_t(1, 1)),
                 std::exception);
}

TEST(VocabularyPoint, FreeFunction) {
    EXPECT_EQ(is_orthogonal_line(point_t(1, 1), point_t(0, 0)), false);
    EXPECT_EQ(is_orthogonal_line(point_t(1, 1), point_t(1, 1)), false);

    EXPECT_EQ(is_orthogonal_line(point_t(1, 1), point_t(2, 1)), true);
}

TEST(VocabularyPoint, Hashing) {
    const auto hash = ankerl::unordered_dense::hash<point_t> {};

    EXPECT_TRUE(hash(point_t(1, 0)) != hash(point_t(0, 0)));
    EXPECT_TRUE(hash(point_t(1, 0)) != hash(point_t(0, 1)));
    EXPECT_TRUE(hash(point_t(1, 1)) == hash(point_t(1, 1)));

    // avalanching
    EXPECT_TRUE(hash(point_t(0, 1)) != 1);
    EXPECT_TRUE(hash(point_t(1, 0)) != 1);
}

}  // namespace logicsim
