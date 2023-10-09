#include "vocabulary/rect.h"

#include <gtest/gtest.h>

namespace logicsim {

TEST(VocabularyRect, Overflow) {
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

}  // namespace logicsim
