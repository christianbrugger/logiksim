#include "vocabulary/line.h"

#include <gtest/gtest.h>

namespace logicsim {

TEST(VocabularyLine, Overflow) {
    // constructor
    EXPECT_THROW(static_cast<void>(line_t {point_t {1, 2}, point_t {1, 2}}),
                 std::exception);
    EXPECT_THROW(static_cast<void>(line_t {point_t {1, 1}, point_t {2, 2}}),
                 std::exception);

    // comparison
    const auto line1 = line_t {point_t {1, 1}, point_t {10, 1}};
    const auto line2 = line_t {point_t {1, 1}, point_t {10, 1}};
    EXPECT_EQ(line1 == line2, true);
}

}  // namespace logicsim
