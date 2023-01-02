
#include "iterator.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/range/has_range_iterator.hpp>

#include <ranges>
#include <vector>

namespace logicsim {

TEST(Iterator, TransformIterator) {
    auto vec = std::vector<int> {1, 2, 3};
    auto proj = [](auto val) { return val * 2; };

    auto begin = TransformIterator(vec.begin(), proj);
    auto end = TransformIterator(vec.end(), proj);
    auto result = std::vector<int>(begin, end);

    ASSERT_THAT(result, testing::ElementsAre(2, 4, 6));
}

TEST(Iterator, TransformView) {
    auto vec = std::vector<int> {1, 2, 3};
    auto proj = [](auto val) { return val * 2; };
    auto transform = TransformView(vec.begin(), vec.end(), proj);

    ASSERT_THAT(transform, testing::ElementsAre(2, 4, 6));
}

TEST(Iterator, TransformViewBoost) {
    auto vec = std::vector<int> {1, 2, 3};
    auto proj = [](auto val) { return val * 2; };
    auto transform = TransformView(vec.begin(), vec.end(), proj);

    static_assert(boost::has_range_iterator<decltype(transform)>::value);
}

TEST(Iterator, TransformViewStl) {
    auto vec = std::vector<int> {1, 2, 3};
    auto proj = [](auto val) { return val * 2; };
    auto transform = TransformView(vec.begin(), vec.end(), proj);

    ASSERT_EQ(std::distance(std::begin(transform), std::end(transform)), 3);
}

TEST(Iterator, TransformViewStlRanges) {
    auto vec = std::vector<int> {1, 2, 3};
    auto proj = [](auto val) { return val * 2; };
    auto transform = TransformView(vec.begin(), vec.end(), proj);

    ASSERT_EQ(std::ranges::distance(transform), 3);
}

TEST(Iterator, TransformViewTypeChange) {
    auto vec = std::vector<int> {1, 2, 3};
    auto proj = [](int val) -> double { return val + 0.5; };

    auto transform = TransformView(vec.begin(), vec.end(), proj);

    static_assert(std::is_same_v<double, decltype(transform)::value_type>);
    ASSERT_THAT(transform, testing::ElementsAre(1.5, 2.5, 3.5));
}

TEST(Iterator, TransformViewSize) {
    auto vec = std::vector<int> {1, 2, 3};
    auto proj = [](auto val) { return val * 2; };
    auto transform = transform_view(std::begin(vec), std::end(vec), proj);

    ASSERT_EQ(transform.size(), 3);
}

TEST(Iterator, TransformViewEmptyFalse) {
    auto vec = std::vector<int> {1, 2, 3};
    auto proj = [](auto val) { return val * 2; };
    auto transform = transform_view(vec, proj);

    ASSERT_EQ(transform.empty(), false);
}

TEST(Iterator, TransformViewEmptyTrue) {
    auto vec = std::vector<int> {};
    auto proj = [](auto val) { return val * 2; };
    auto transform = transform_view(vec, proj);

    ASSERT_EQ(transform.empty(), true);
}

}  // namespace logicsim
