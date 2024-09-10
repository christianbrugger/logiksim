#include "vocabulary/point_fine.h"

#include <gtest/gtest.h>

namespace logicsim {

TEST(VocabularyPointFine, Operators) {
    // add
    EXPECT_EQ(point_fine_t(10, 20) + point_fine_t(1, 2), point_fine_t(11, 22));
    {
        auto offset = point_fine_t {10, 20};
        offset += point_fine_t {1, 2};
        EXPECT_EQ(offset, point_fine_t(11, 22));
    }

    // substract
    EXPECT_EQ(point_fine_t(10, 20) - point_fine_t(1, 2), point_fine_t(9, 18));
    {
        auto offset = point_fine_t {10, 20};
        offset -= point_fine_t {1, 2};
        EXPECT_EQ(offset, point_fine_t(9, 18));
    }
}

TEST(VocabularyPointFine, FreeFunction) {
    EXPECT_EQ(is_orthogonal_line(point_fine_t(1, 1), point_fine_t(0, 0)), false);
    EXPECT_EQ(is_orthogonal_line(point_fine_t(1, 1), point_fine_t(1, 1)), false);

    EXPECT_EQ(is_orthogonal_line(point_fine_t(1, 1), point_fine_t(2, 1)), true);
}

}  // namespace logicsim
