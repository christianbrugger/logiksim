
#include "core/algorithm/fmt_join.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

namespace logicsim {

TEST(Format, VectorJoin) {
    auto vec = std::vector<int> {1, 2, 3};

    EXPECT_EQ(fmt_join(", ", vec), "1, 2, 3");
    EXPECT_EQ(fmt_join(" - ", vec), "1 - 2 - 3");
    EXPECT_EQ(fmt_join(":", vec, "{:03d}"), "001:002:003");
}

}  // namespace logicsim
