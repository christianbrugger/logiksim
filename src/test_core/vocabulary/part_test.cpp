#include "core/vocabulary/part.h"

#include <gtest/gtest.h>

namespace logicsim {

TEST(VocabularyPart, Overflow) {
    // constructor
    EXPECT_THROW(static_cast<void>(part_t {1, 1}), std::exception);
    EXPECT_THROW(static_cast<void>(part_t {10, 5}), std::exception);
    EXPECT_THROW(static_cast<void>(part_t {-1, 0}), std::exception);

    // comparison
    {
        const auto test = part_t {1, 2} == part_t {1, 2};
        EXPECT_EQ(test, true);
    }
    {
        const auto test = part_t {1, 2} < part_t {1, 3};
        EXPECT_EQ(test, true);
    }
    {
        const auto test = part_t {1, 2} >= part_t {1, 3};
        EXPECT_EQ(test, false);
    }
}

}  // namespace logicsim
