#include "vocabulary/rect_fine.h"

#include <gtest/gtest.h>

namespace logicsim {

TEST(VocabularyRectFine, PointFineOperator) {
    // add
    EXPECT_EQ(rect_fine_t(point_t(-10, -20), point_t(10, 20)) + point_fine_t(1, 2),
              rect_fine_t(point_t(-9, -18), point_t(11, 22)));
    EXPECT_EQ(point_fine_t(1, 2) + rect_fine_t(point_t(-10, -20), point_t(10, 20)),
              rect_fine_t(point_t(-9, -18), point_t(11, 22)));
    {
        auto rect = rect_fine_t(point_t(-10, -20), point_t(10, 20));
        rect += point_fine_t {1, 2};
        EXPECT_EQ(rect, rect_fine_t(point_t(-9, -18), point_t(11, 22)));
    }

    // substract
    EXPECT_EQ(rect_fine_t(point_t(-10, -20), point_t(10, 20)) - point_fine_t(1, 2),
              rect_fine_t(point_t(-11, -22), point_t(9, 18)));
    {
        auto rect = rect_fine_t(point_t(-10, -20), point_t(10, 20));
        rect -= point_fine_t(1, 2);
        EXPECT_EQ(rect, rect_fine_t(point_t(-11, -22), point_t(9, 18)));
    }
}

TEST(VocabularyRectFine, PointOperator) {
    // add
    EXPECT_EQ(rect_fine_t(point_t(-10, -20), point_t(10, 20)) + point_t(1, 2),
              rect_fine_t(point_t(-9, -18), point_t(11, 22)));
    EXPECT_EQ(point_t(1, 2) + rect_fine_t(point_t(-10, -20), point_t(10, 20)),
              rect_fine_t(point_t(-9, -18), point_t(11, 22)));
    {
        auto rect = rect_fine_t(point_t(-10, -20), point_t(10, 20));
        rect += point_t {1, 2};
        EXPECT_EQ(rect, rect_fine_t(point_t(-9, -18), point_t(11, 22)));
    }

    // substract
    EXPECT_EQ(rect_fine_t(point_t(-10, -20), point_t(10, 20)) - point_t(1, 2),
              rect_fine_t(point_t(-11, -22), point_t(9, 18)));
    {
        auto rect = rect_fine_t(point_t(-10, -20), point_t(10, 20));
        rect -= point_t(1, 2);
        EXPECT_EQ(rect, rect_fine_t(point_t(-11, -22), point_t(9, 18)));
    }
}

}  // namespace logicsim
