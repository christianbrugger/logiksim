
#include "algorithm/range_extended.h"

#include "algorithm/to_vector.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fmt/core.h>
#include <gsl/gsl>

#include <algorithm>

namespace logicsim {

TEST(AlgorithmRangeExtended, SimpleSize) {
    const auto r = range_extended<int, int>(10);

    EXPECT_EQ(std::size(r), 10);
}

TEST(AlgorithmRangeExtended, SimpleValues) {
    const auto r = range_extended<int, int>(10);

    auto result = std::vector<int> {};
    std::ranges::copy(r, std::back_inserter(result));

    ASSERT_THAT(result, testing::ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9));
}

namespace {
struct CustomType {
    using value_type = int8_t;
    value_type value;

    auto operator==(const CustomType&) const -> bool = default;
};
}  // namespace

}  // namespace logicsim

namespace logicsim {

TEST(AlgorithmRangeExtended, CustomTypeSize) {
    const auto r = range_extended<CustomType>(10);

    EXPECT_EQ(std::size(r), 10);
}

TEST(AlgorithmRangeExtended, CustomTypeValues) {
    const auto r = range_extended<CustomType>(3);

    ASSERT_THAT(to_vector(r),
                testing::ElementsAre(CustomType(0), CustomType(1), CustomType(2)));
}

TEST(AlgorithmRangeExtended, FullRange) {
    constexpr auto max_value =
        CustomType {std::numeric_limits<CustomType::value_type>::max()};
    constexpr auto count = static_cast<std::size_t>(max_value.value) + 1;

    const auto r = range_extended<CustomType>(count);
    const auto result = to_vector(r);

    EXPECT_EQ(result.size(), count);
    EXPECT_EQ(result.front(), CustomType {0});
    EXPECT_EQ(result.back(), max_value);
}

TEST(AlgorithmRangeExtended, TypeMatches) {
    auto r = range_extended<CustomType>(10);

    static_assert(std::is_same_v<decltype(r), range_extended_t<CustomType>>);
}

}  // namespace logicsim
