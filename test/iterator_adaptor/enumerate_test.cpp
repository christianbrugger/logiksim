
#include "iterator_adaptor/enumerate.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <ranges>
#include <utility>
#include <vector>

namespace logicsim {

TEST(IteratorAdaptorEnumerate, View) {
    auto container = std::vector<int> {1, 2, 3};
    auto expected = std::array<std::pair<std::size_t, int>, 3> {
        std::pair {0, 1}, std::pair {1, 2}, std::pair {2, 3}};

    auto view = enumerate(container);

    EXPECT_TRUE(std::ranges::equal(view, expected));
    EXPECT_TRUE(std::ranges::equal(view.begin(), view.end(),  //
                                   expected.begin(), expected.end()));
}

TEST(IteratorAdaptorEnumerate, EmptyView) {
    auto container = std::vector<int> {};
    auto expected = std::array<std::pair<std::size_t, int>, 0> {};

    auto view = enumerate(container);

    EXPECT_TRUE(std::ranges::equal(view, expected));
    EXPECT_TRUE(std::ranges::equal(view.begin(), view.end(),  //
                                   expected.begin(), expected.end()));
}

TEST(IteratorAdaptorEnumerate, ViewInt) {
    auto container = std::vector<int> {1, 2, 3};
    auto expected = std::array<std::pair<int, int>, 3> {
        std::pair {0, 1}, std::pair {1, 2}, std::pair {2, 3}};

    auto view = enumerate<int>(container);

    EXPECT_TRUE(std::ranges::equal(view, expected));
    EXPECT_TRUE(std::ranges::equal(view.begin(), view.end(),  //
                                   expected.begin(), expected.end()));
}

TEST(IteratorAdaptorEnumerate, ConstView) {
    const auto container = std::vector<int> {1, 2, 3};
    const auto expected = std::array<std::pair<std::size_t, int>, 3> {
        std::pair {0, 1}, std::pair {1, 2}, std::pair {2, 3}};

    auto view = enumerate(container);

    EXPECT_TRUE(std::ranges::equal(view, expected));
    EXPECT_TRUE(std::ranges::equal(view.begin(), view.end(),  //
                                   expected.begin(), expected.end()));
}

// TEST(IteratorAdaptorEnumerate, ModifyingView) {
//     auto container = std::vector<int> {5, 5, 5};
//     auto view = enumerate(container);
//
//     // before
//
//     auto container_before = std::vector<int> {5, 5, 5};
//     const auto expected_before = std::array<std::pair<int, int>, 3> {
//         std::pair {0, 5}, std::pair {1, 5}, std::pair {2, 5}};
//
//     EXPECT_TRUE(std::ranges::equal(container, container_before));
//     EXPECT_TRUE(std::ranges::equal(view, expected_before));
//     EXPECT_TRUE(std::ranges::equal(view.begin(), view.end(),  //
//                                    expected_before.begin(), expected_before.end()));
//
//     // modify
//
//     // for (auto &pair : view) {
//     //     pair.second = pair.first;
//     // }
//
//     // after
//     auto container_after = std::vector<int> {1, 2, 3};
//     const auto expected_after = std::array<std::pair<int, int>, 3> {
//         std::pair {0, 0}, std::pair {1, 1}, std::pair {2, 2}};
//
//     EXPECT_TRUE(std::ranges::equal(container, container_after));
//     EXPECT_TRUE(std::ranges::equal(view, expected_after));
//     EXPECT_TRUE(std::ranges::equal(view.begin(), view.end(),  //
//                                    expected_after.begin(), expected_after.end()));
// }

TEST(IteratorAdaptorEnumerate, Owning) {
    auto range = [] { return enumerate(std::vector<int> {1, 2, 3}); }();

    auto expected = std::array<std::pair<std::size_t, int>, 3> {
        std::pair {0, 1}, std::pair {1, 2}, std::pair {2, 3}};

    // ASSERT_EQ(std::ranges::equal(range, expected), true);

    EXPECT_TRUE(std::ranges::equal(range, expected));
    EXPECT_TRUE(std::ranges::equal(range.begin(), range.end(),  //
                                   expected.begin(), expected.end()));
}

TEST(IteratorAdaptorEnumerate, OwningConst) {
    const auto range = [] { return enumerate(std::vector<int> {1, 2, 3}); }();

    const auto expected = std::array<std::pair<std::size_t, int>, 3> {
        std::pair {0, 1}, std::pair {1, 2}, std::pair {2, 3}};

    EXPECT_TRUE(std::ranges::equal(range, expected));
    EXPECT_TRUE(std::ranges::equal(range.begin(), range.end(),  //
                                   expected.begin(), expected.end()));
}

//
// Custom Type
//

namespace {
struct CustomType {
    int16_t value;

    using difference_type = std::ptrdiff_t;

    auto operator==(const CustomType& other) const -> bool = default;

    auto operator++() -> CustomType& {
        ++value;
        return *this;
    }

    auto operator++(int) -> CustomType {
        auto temp = *this;
        operator++();
        return temp;
    }
};
}  // namespace

TEST(IteratorAdaptorEnumerate, CustomType) {
    static_assert(std::weakly_incrementable<CustomType>);

    auto container = std::vector<int> {1, 2, 3};
    auto expected = std::array<std::pair<CustomType, int>, 3> {
        std::pair {CustomType {0}, 1}, std::pair {CustomType {1}, 2},
        std::pair {CustomType {2}, 3}};

    auto view = enumerate<CustomType>(container);

    EXPECT_TRUE(std::ranges::equal(view, expected));
    EXPECT_TRUE(std::ranges::equal(view.begin(), view.end(),  //
                                   expected.begin(), expected.end()));
}

}  // namespace logicsim
