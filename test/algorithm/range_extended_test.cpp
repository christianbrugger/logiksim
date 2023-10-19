
#include "algorithm/range_extended.h"

#include "logging.h"
#include "type_trait/numeric_limits_template.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fmt/core.h>
#include <gsl/gsl>

#include <ranges>

namespace logicsim {

TEST(AlgorithmRangeExtended, SimpleSize) {
    EXPECT_EQ(std::size(range_extended<int, int>(10)), 10);
}

TEST(AlgorithmRangeExtended, SimpleValues) {
    const auto r = range_extended<int, int>(10);

    const auto result = std::vector<int>(r.begin(), r.end());

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
    EXPECT_EQ(std::size(range_extended<CustomType>(10)), 10);
}

TEST(AlgorithmRangeExtended, CustomTypeValues) {
    const auto r = range_extended<CustomType>(3);

    const auto result = std::vector<CustomType>(r.begin(), r.end());

    ASSERT_THAT(result,
                testing::ElementsAre(CustomType(0), CustomType(1), CustomType(2)));
}

TEST(AlgorithmRangeExtended, FullRange) {
    constexpr auto max_value =
        CustomType {std::numeric_limits<CustomType::value_type>::max()};
    constexpr auto count = static_cast<std::size_t>(max_value.value) + 1;

    const auto r = range_extended<CustomType>(count);
    const auto result = std::vector<CustomType>(r.begin(), r.end());

    EXPECT_EQ(result.size(), count);
    EXPECT_EQ(result.front(), CustomType {0});
    EXPECT_EQ(result.back(), max_value);
}

TEST(AlgorithmRangeExtended, TypeMatches) {
    auto r = range_extended<CustomType>(10);

    static_assert(std::is_same_v<decltype(r), range_extended_t<CustomType>>);
}

}  // namespace logicsim
