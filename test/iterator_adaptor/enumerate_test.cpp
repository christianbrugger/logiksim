
#include "iterator_adaptor/enumerate.h"

#include <gtest/gtest.h>

#include <gsl/gsl>

#include <algorithm>
#include <array>
#include <cstdint>
#include <ranges>
#include <utility>
#include <vector>

namespace logicsim {

namespace {
template <typename C>
auto pair_equal(const std::pair<C, int&>& a, const std::pair<C, int>& b) -> bool {
    return a.first == b.first && a.second == b.second;
}

template <typename C>
auto const_pair_equal(const std::pair<C, const int&>& a, const std::pair<C, int>& b)
    -> bool {
    return a.first == b.first && a.second == b.second;
}
}  // namespace

TEST(IteratorAdaptorEnumerate, View) {
    auto container = std::vector<int> {1, 2, 3};
    auto expected = std::array<std::pair<std::size_t, int>, 3> {
        std::pair {0, 1}, std::pair {1, 2}, std::pair {2, 3}};

    auto view = enumerate(container);

    static_assert(std::ranges::sized_range<decltype(view)>);
    static_assert(std::sized_sentinel_for<decltype(view.end()), decltype(view.begin())>);

    EXPECT_EQ(std::size(view), expected.size());
    EXPECT_EQ(view.empty(), false);

    EXPECT_TRUE(std::ranges::equal(view, expected, pair_equal<std::size_t>));
    EXPECT_TRUE(std::ranges::equal(view.begin(), view.end(),  //
                                   expected.begin(), expected.end(),
                                   pair_equal<std::size_t>));
}

TEST(IteratorAdaptorEnumerate, EmptyView) {
    auto container = std::vector<int> {};
    auto expected = std::array<std::pair<std::size_t, int>, 0> {};

    auto view = enumerate(container);

    EXPECT_EQ(std::size(view), expected.size());
    EXPECT_EQ(view.empty(), true);

    EXPECT_TRUE(std::ranges::equal(view, expected, pair_equal<std::size_t>));
    EXPECT_TRUE(std::ranges::equal(view.begin(), view.end(),  //
                                   expected.begin(), expected.end(),
                                   pair_equal<std::size_t>));
}

TEST(IteratorAdaptorEnumerate, ConstView) {
    const auto container = std::vector<int> {1, 2, 3};
    const auto expected = std::array<std::pair<std::size_t, int>, 3> {
        std::pair {0, 1}, std::pair {1, 2}, std::pair {2, 3}};

    auto view = enumerate(container);

    EXPECT_TRUE(std::ranges::equal(view, expected, const_pair_equal<std::size_t>));
    EXPECT_TRUE(std::ranges::equal(view.begin(), view.end(),  //
                                   expected.begin(), expected.end(),
                                   const_pair_equal<std::size_t>));
}

TEST(IteratorAdaptorEnumerate, ViewInt) {
    auto container = std::vector<int> {1, 2, 3};
    auto expected = std::array<std::pair<int, int>, 3> {
        std::pair {0, 1}, std::pair {1, 2}, std::pair {2, 3}};

    auto view = enumerate<int>(container);

    EXPECT_TRUE(std::ranges::equal(view, expected, pair_equal<int>));
    EXPECT_TRUE(std::ranges::equal(view.begin(), view.end(),  //
                                   expected.begin(), expected.end(), pair_equal<int>));
}

TEST(IteratorAdaptorEnumerate, SentinelConvertible) {
    auto container = std::vector<int> {};

    auto view = enumerate(container);

    auto begin = view.begin();
    using iterator = decltype(begin);

    auto sentinel = view.end();
    auto end = static_cast<iterator>(sentinel);

    EXPECT_EQ(begin, sentinel);
    EXPECT_EQ(begin, end);
    EXPECT_EQ(sentinel, end);
}

//
// Modify
//

TEST(IteratorAdaptorEnumerate, ModifyingView) {
    auto container = std::vector<int> {5, 5, 5};
    auto view = enumerate(container);

    // before

    auto container_before = std::vector<int> {5, 5, 5};
    const auto expected_before = std::array<std::pair<int, int>, 3> {
        std::pair {0, 5}, std::pair {1, 5}, std::pair {2, 5}};

    EXPECT_TRUE(std::ranges::equal(container, container_before));
    EXPECT_TRUE(std::ranges::equal(view, expected_before, pair_equal<std::size_t>));
    EXPECT_TRUE(std::ranges::equal(view.begin(), view.end(),  //
                                   expected_before.begin(), expected_before.end(),
                                   pair_equal<std::size_t>));

    // modify
    for (auto pair : view) {
        pair.second = gsl::narrow<int>(pair.first);
    }

    // after
    auto container_after = std::vector<int> {0, 1, 2};
    const auto expected_after = std::array<std::pair<int, int>, 3> {
        std::pair {0, 0}, std::pair {1, 1}, std::pair {2, 2}};

    EXPECT_TRUE(std::ranges::equal(container, container_after));
    EXPECT_TRUE(std::ranges::equal(view, expected_after, pair_equal<std::size_t>));
    EXPECT_TRUE(std::ranges::equal(view.begin(), view.end(),  //
                                   expected_after.begin(), expected_after.end(),
                                   pair_equal<std::size_t>));
}

//
// Owning
//

TEST(IteratorAdaptorEnumerate, Owning) {
    auto range = [] { return enumerate(std::vector<int> {1, 2, 3}); }();

    auto expected = std::array<std::pair<std::size_t, int>, 3> {
        std::pair {0, 1}, std::pair {1, 2}, std::pair {2, 3}};

    static_assert(std::ranges::sized_range<decltype(range)>);
    static_assert(
        std::sized_sentinel_for<decltype(range.end()), decltype(range.begin())>);

    EXPECT_EQ(std::size(range), expected.size());
    EXPECT_EQ(range.empty(), false);

    EXPECT_TRUE(std::ranges::equal(range, expected, pair_equal<std::size_t>));
    EXPECT_TRUE(std::ranges::equal(range.begin(), range.end(),  //
                                   expected.begin(), expected.end(),
                                   pair_equal<std::size_t>));
}

TEST(IteratorAdaptorEnumerate, OwningConst) {
    const auto range = [] { return enumerate(std::vector<int> {1, 2, 3}); }();

    const auto expected = std::array<std::pair<std::size_t, int>, 3> {
        std::pair {0, 1}, std::pair {1, 2}, std::pair {2, 3}};

    EXPECT_EQ(std::size(range), expected.size());
    EXPECT_EQ(range.empty(), false);

    EXPECT_TRUE(std::ranges::equal(range, expected, const_pair_equal<std::size_t>));
    EXPECT_TRUE(std::ranges::equal(range.begin(), range.end(),  //
                                   expected.begin(), expected.end(),
                                   const_pair_equal<std::size_t>));
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

    EXPECT_TRUE(std::ranges::equal(view, expected, pair_equal<CustomType>));
    EXPECT_TRUE(std::ranges::equal(view.begin(), view.end(),  //
                                   expected.begin(), expected.end(),
                                   pair_equal<CustomType>));
}

}  // namespace logicsim
