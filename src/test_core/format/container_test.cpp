
#include "format/container.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fmt/core.h>

#include <vector>

namespace logicsim {

TEST(Format, VectorFormat) {
    auto vec = std::vector<int> {1, 2, 3};

    EXPECT_EQ(fmt::format("{}", vec), "[1, 2, 3]");
}

TEST(Format, VectorFormatNoBracket) {
    auto vec = std::vector<int> {1, 2, 3};

    EXPECT_EQ(fmt::format("{:n}", vec), "1, 2, 3");
}

}  // namespace logicsim
