
#include "range.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fmt/chrono.h>
#include <fmt/core.h>

#include <chrono>
#include <ranges>

namespace logicsim {

TEST(Range, SizeIsCorrect) {
    EXPECT_EQ(std::size(range(10)), 10);
    EXPECT_EQ(std::size(range(15)), 15);
    EXPECT_EQ(std::size(range(10, 15)), 5);
    EXPECT_EQ(std::size(range(-10, 0)), 10);
}

TEST(Range, NegativeRangeIsEmpty) {
    EXPECT_EQ(std::size(range(10, 5)), 0);
    EXPECT_EQ(std::size(range(-10)), 0);
}

TEST(Range, IteratorDistanceCorrect) {
    // this iterates the iterator
    EXPECT_EQ(std::ranges::distance(range(10)), 10);
    EXPECT_EQ(std::ranges::distance(range(15)), 15);
    EXPECT_EQ(std::ranges::distance(range(0)), 0);
    EXPECT_EQ(std::ranges::distance(range(-10, 0)), 10);
}

TEST(Range, NegativeRangeDistanceZero) {
    // this iterates the iterator
    EXPECT_EQ(std::ranges::distance(range(10, 5)), 0);
    EXPECT_EQ(std::ranges::distance(range(-10)), 0);
}

TEST(Range, EmptyAttribute) {
    // this iterates the iterator
    EXPECT_EQ(std::empty(range(10, 5)), true);
    EXPECT_EQ(std::empty(range(0)), true);
    EXPECT_EQ(std::empty(range(-10)), true);

    EXPECT_EQ(std::empty(range(10)), false);
    EXPECT_EQ(std::empty(range(0, 5)), false);
    EXPECT_EQ(std::empty(range(0, 1)), false);
}

TEST(Range, ElementsCheck) {
    ASSERT_THAT(range(5), testing::ElementsAre(0, 1, 2, 3, 4));
    ASSERT_THAT(range(5, 10), testing::ElementsAre(5, 6, 7, 8, 9));

    ASSERT_THAT(range(-1), testing::ElementsAre());
}

TEST(Range, FormatFunction) {
    ASSERT_EQ(range(5).format(), "range(0, 5)");
}

TEST(Range, FmtFormatting) {
    ASSERT_EQ(fmt::format("{}", range(0, 10)), "range(0, 10)");
    ASSERT_EQ(fmt::format("{}", range(10)), "range(0, 10)");
    ASSERT_EQ(fmt::format("{}", range(5, 7)), "range(5, 7)");
    ASSERT_EQ(fmt::format("{}", range(-2, -100)), "range(-2, -100)");
}

TEST(Range, StlDistance) {
    // this iterates the iterator
    {
        const auto r = range(10);
        EXPECT_EQ(std::distance(r.begin(), r.end()), 10);
    }
    {
        const auto r = range(0);
        EXPECT_EQ(std::distance(r.begin(), r.end()), 0);
    }
    {
        const auto r = range(-10, 0);
        EXPECT_EQ(std::distance(r.begin(), r.end()), 10);
    }
    {
        const auto r = range(-10);
        EXPECT_EQ(std::distance(r.begin(), r.end()), 0);
    }
}

namespace {

struct StrongType {
    int value;

    constexpr auto operator++() noexcept -> StrongType& {
        ++value;
        return *this;
    }

    constexpr auto operator++(int) noexcept -> void {
        ++value;
    }

    using difference_type = range_difference_t<decltype(value)>;

    explicit operator difference_type() const {
        return value;
    }

    auto operator<=>(const StrongType&) const = default;
};

TEST(Range, CustomClass) {
    ASSERT_THAT(range(StrongType {2}, StrongType {5}),
                testing::ElementsAre(StrongType {2}, StrongType {3}, StrongType {4}));

    ASSERT_THAT(range(StrongType {2}),
                testing::ElementsAre(StrongType {0}, StrongType {1}));

    ASSERT_THAT(range(StrongType {-2}), testing::ElementsAre());

    ASSERT_EQ(std::empty(range(StrongType {-2})), true);
    ASSERT_EQ(std::empty(range(StrongType {2})), false);

    // auto v = std::ranges::iota_view<StrongType> {};

    // auto r [[maybe_unused]] = v.size();

    ASSERT_EQ(std::size(range(StrongType {0})), 0);
    ASSERT_EQ(std::size(range(StrongType {5})), 5);
    ASSERT_EQ(std::size(range(StrongType {5}, StrongType {15})), 10);
}

//
// Reverse Range
//

TEST(RangeReverse, SizeIsCorrect) {
    ASSERT_THAT(reverse_range(5), testing::ElementsAre(4, 3, 2, 1, 0));
    ASSERT_THAT(reverse_range(0, 5), testing::ElementsAre(4, 3, 2, 1, 0));

    ASSERT_THAT(reverse_range(1), testing::ElementsAre(0));
    ASSERT_THAT(reverse_range(0), testing::ElementsAre());

    EXPECT_EQ(std::size(reverse_range(0, 10)), 10);
    EXPECT_EQ(std::size(reverse_range(15)), 15);

    ASSERT_EQ(fmt::format("{}", reverse_range(1, 10)), "reverse_range(1, 10)");
    ASSERT_EQ(fmt::format("{}", reverse_range(10)), "reverse_range(0, 10)");
}

//
// Step Range
//

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

}  // namespace

}  // namespace logicsim