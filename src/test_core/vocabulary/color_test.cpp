#include "core/vocabulary/color.h"

#include <gtest/gtest.h>

namespace logicsim {

TEST(VocabularyColor, Construction) {
    const auto c = color_t {1, 2, 3, 4};

    EXPECT_EQ(c.r(), 1);
    EXPECT_EQ(c.g(), 2);
    EXPECT_EQ(c.b(), 3);
    EXPECT_EQ(c.a(), 4);
}

}  // namespace logicsim
