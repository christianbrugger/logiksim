
#include "core/iterator_adaptor/transform_view.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/range/has_range_iterator.hpp>

#include <functional>
#include <ranges>
#include <vector>

namespace logicsim {

TEST(Iterator, TransformViewSimple) {
    auto vec = std::vector<int> {1, 2, 3};
    auto proj = [](auto val) { return val * 2; };
    auto transform = detail::TransformView(vec.begin(), vec.end(), proj);

    ASSERT_THAT(transform, testing::ElementsAre(2, 4, 6));
}

TEST(Iterator, TransformViewBoost) {
    auto vec = std::vector<int> {1, 2, 3};
    auto proj = [](auto val) { return val * 2; };
    auto transform = detail::TransformView(vec.begin(), vec.end(), proj);

    static_assert(boost::has_range_iterator<decltype(transform)>::value);
}

TEST(Iterator, TransformViewStl) {
    auto vec = std::vector<int> {1, 2, 3};
    auto proj = [](auto val) { return val * 2; };
    auto transform = detail::TransformView(vec.begin(), vec.end(), proj);

    ASSERT_EQ(std::distance(std::begin(transform), std::end(transform)), 3);
}

TEST(Iterator, TransformViewStlRanges) {
    auto vec = std::vector<int> {1, 2, 3};
    auto proj = [](auto val) { return val * 2; };
    auto transform = detail::TransformView(vec.begin(), vec.end(), proj);

    ASSERT_EQ(std::ranges::distance(transform), 3);
}

TEST(Iterator, TransformViewTypeChange) {
    auto vec = std::vector<int> {1, 2, 3};
    auto proj = [](int val) -> double { return val + 0.5; };

    auto transform = detail::TransformView(vec.begin(), vec.end(), proj);

    static_assert(std::is_same_v<double, typename decltype(transform)::value_type>);
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

auto proj_times_two(int val) -> int {
    return val * 2;
}

TEST(Iterator, TransformViewPassFunction) {
    auto vec = std::vector<int> {1, 2, 3};
    auto transform = transform_view(vec, proj_times_two);
    ASSERT_THAT(transform, testing::ElementsAre(2, 4, 6));
}

TEST(Iterator, TransformViewPassStdFunction) {
    auto vec = std::vector<int> {1, 2, 3};

    auto func = std::function(proj_times_two);
    auto transform = transform_view(vec, func);

    ASSERT_THAT(transform, testing::ElementsAre(2, 4, 6));
}

struct MemberTest {
    int val {};

    auto proj_times_three() -> int {
        return val * 3;
    }
};

TEST(Iterator, TransformViewPassMemberFunctionViaStdFunction) {
    auto vec = std::vector<MemberTest> {MemberTest {1}, MemberTest {2}, MemberTest {3}};

    auto func = std::function<int(MemberTest &)>(&MemberTest::proj_times_three);
    auto transform = transform_view(vec, func);

    ASSERT_THAT(transform, testing::ElementsAre(3, 6, 9));
}

TEST(Iterator, TransformViewPassMemberFunctionDirectly) {
    auto vec = std::vector<MemberTest> {MemberTest {1}, MemberTest {2}, MemberTest {3}};

    auto transform = transform_view(vec, &MemberTest::proj_times_three);

    ASSERT_THAT(transform, testing::ElementsAre(3, 6, 9));
}

TEST(Iterator, TransformViewLambda) {
    auto vec = std::vector<int> {1, 2, 3};

    auto proj = [offset = int {10}](int v) { return v + offset; };
    auto transform = transform_view(vec, proj);

    ASSERT_TRUE(std::ranges::equal(transform, std::vector<int> {11, 12, 13}));

    ASSERT_THAT(transform, testing::ElementsAre(11, 12, 13));
}

}  // namespace logicsim
