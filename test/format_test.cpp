
#include "format.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fmt/core.h>

#include <vector>

TEST(Format, VectorFormat) {
    auto vec = std::vector<int> {1, 2, 3};

    EXPECT_EQ(fmt::format("{}", vec), "[1, 2, 3]");
}

TEST(Format, VectorFormatNoBracket) {
    auto vec = std::vector<int> {1, 2, 3};

    EXPECT_EQ(fmt::format("{:n}", vec), "1, 2, 3");
}

TEST(Format, VectorJoin) {
    auto vec = std::vector<int> {1, 2, 3};

    EXPECT_EQ(logicsim::fmt_join(", ", vec), "1, 2, 3");
    EXPECT_EQ(logicsim::fmt_join(" - ", vec), "1 - 2 - 3");
    EXPECT_EQ(logicsim::fmt_join(":", vec, "{:03d}"), "001:002:003");
}
