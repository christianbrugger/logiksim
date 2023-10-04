
#include "algorithm/range_step.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fmt/core.h>

#include <ranges>

namespace logicsim {

TEST(RangeStep, SizeIsCorrect) {
    EXPECT_EQ(std::size(range(0, 10, 2)), 5);
    EXPECT_EQ(std::size(range(0, 15, 10)), 2);
    EXPECT_EQ(std::size(range(0, 3, 10)), 1);

    EXPECT_EQ(std::size(range(15, 10, -2)), 3);
    EXPECT_EQ(std::size(range(15, 10, -1)), 5);
}

TEST(RangeStep, NegativeRangeIsEmpty) {
    EXPECT_EQ(std::size(range(10, 10, 1)), 0);
    EXPECT_EQ(std::size(range(10, 10, 2)), 0);
    EXPECT_EQ(std::size(range(10, 10, -2)), 0);

    EXPECT_EQ(std::size(range(10, 11, -2)), 0);
    EXPECT_EQ(std::size(range(10, 5, 2)), 0);
    EXPECT_EQ(std::size(range(0, 10, -2)), 0);
}

TEST(RangeStep, IteratorDistanceCorrect) {
    // this iterates the iterator
    EXPECT_EQ(std::ranges::distance(range(0, 10, 2)), 5);
    EXPECT_EQ(std::ranges::distance(range(0, 15, 10)), 2);
    EXPECT_EQ(std::ranges::distance(range(0, 0, 1)), 0);
    EXPECT_EQ(std::ranges::distance(range(10, 0, -1)), 10);
}

TEST(RangeStep, NegativeRangeDistanceZero) {
    // this iterates the iterator
    EXPECT_EQ(std::ranges::distance(range(10, 5, 2)), 0);
    EXPECT_EQ(std::ranges::distance(range(0, 10, -7)), 0);
}

TEST(RangeStep, EmptyAttribute) {
    // this iterates the iterator
    EXPECT_EQ(std::empty(range(10, 5, 2)), true);
    EXPECT_EQ(std::empty(range(0, 0, 2)), true);
    EXPECT_EQ(std::empty(range(0, -10, 5)), true);
    EXPECT_EQ(std::empty(range(0, 10, -5)), true);

    EXPECT_EQ(std::empty(range(0, 10, 2)), false);
    EXPECT_EQ(std::empty(range(0, 5, 7)), false);
    EXPECT_EQ(std::empty(range(0, -1, -1)), false);
}

TEST(RangeStep, ElementsCheck) {
    ASSERT_THAT(range(0, 5, 1), testing::ElementsAre(0, 1, 2, 3, 4));
    ASSERT_THAT(range(0, 5, 5), testing::ElementsAre(0));
    ASSERT_THAT(range(0, 5, 3), testing::ElementsAre(0, 3));

    ASSERT_THAT(range(10, 5, -1), testing::ElementsAre(10, 9, 8, 7, 6));
    ASSERT_THAT(range(10, 5, -2), testing::ElementsAre(10, 8, 6));
}

TEST(RangeStep, FmtFormatting) {
    ASSERT_EQ(fmt::format("{}", range(0, 10, 1)), "range(0, 10, 1)");
    ASSERT_EQ(fmt::format("{}", range(-2, 10, 2)), "range(-2, 10, 2)");
    ASSERT_EQ(fmt::format("{}", range(5, 7, -10)), "range(5, 7, -10)");
}

}  // namespace logicsim