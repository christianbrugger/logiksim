#include "vocabulary/line_fine.h"

#include <gtest/gtest.h>

namespace logicsim {

TEST(VocabularyLineFine, Overflow) {
    // constructor
    EXPECT_THROW(static_cast<void>(line_fine_t {point_fine_t {1, 2}, point_t {1, 2}}),
                 std::exception);
    EXPECT_THROW(static_cast<void>(line_fine_t {point_t {1, 1}, point_fine_t {2, 2}}),
                 std::exception);

    // comparison
    const auto line1 = line_fine_t {point_fine_t {1, 1}, point_t {10, 1}};
    const auto line2 = line_fine_t {point_t {1, 1}, point_fine_t {10, 1}};
    const auto line3 = line_fine_t {line_t {point_t {1, 1}, point_t {10, 1}}};
    const auto line4 = line_fine_t {ordered_line_t {point_t {1, 1}, point_t {10, 1}}};
    EXPECT_EQ(line1 == line2, true);
    EXPECT_EQ(line2 == line3, true);
    EXPECT_EQ(line3 == line4, true);
}

}  // namespace logicsim
