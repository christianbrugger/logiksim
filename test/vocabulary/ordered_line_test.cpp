#include "vocabulary/ordered_line.h"

#include <gtest/gtest.h>

namespace logicsim {

TEST(VocabularyOrderedLine, Overflow) {
    // constructor
    EXPECT_THROW(static_cast<void>(ordered_line_t {point_t {1, 2}, point_t {1, 2}}),
                 std::exception);
    EXPECT_THROW(static_cast<void>(ordered_line_t {point_t {1, 1}, point_t {2, 2}}),
                 std::exception);
    EXPECT_THROW(static_cast<void>(ordered_line_t {point_t {2, 1}, point_t {1, 1}}),
                 std::exception);
    {
        const auto line1 = ordered_line_t {line_t {point_t {10, 1}, point_t {1, 1}}};
        const auto line2 = ordered_line_t {point_t {1, 1}, point_t {10, 1}};
        EXPECT_EQ(line1 == line2, true);
    }

    // comparison
    {
        const auto line1 = ordered_line_t {point_t {1, 1}, point_t {10, 1}};
        const auto line2 = ordered_line_t {point_t {1, 1}, point_t {10, 1}};
        EXPECT_EQ(line1 == line2, true);
    }
    {
        const auto line1 = ordered_line_t {point_t {1, 1}, point_t {10, 1}};
        const auto line2 = ordered_line_t {point_t {1, 1}, point_t {11, 1}};
        EXPECT_EQ(line1 < line2, true);
        EXPECT_EQ(line1 >= line2, false);
    }
    {
        const auto line1 = ordered_line_t {point_t {1, 1}, point_t {1, 10}};
        const auto line2 = ordered_line_t {point_t {1, 2}, point_t {1, 10}};
        EXPECT_EQ(line1 < line2, true);
        EXPECT_EQ(line1 >= line2, false);
    }

    // line_t conversion
    {
        const auto p0 = point_t {1, 1};
        const auto p1 = point_t {10, 1};
        const auto test0 = line_t {p0, p1} == line_t {ordered_line_t {p0, p1}};
        const auto test1 = line_t {p1, p0} == line_t {ordered_line_t {p0, p1}};
        EXPECT_EQ(test0, true);
        EXPECT_EQ(test1, false);
    }
}

}  // namespace logicsim
