
#include "algorithm.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

namespace logicsim {

TEST(Algorithm, TransformToVector) {
    auto vec = std::vector<int> {1, 2, 3};
    auto func = [](auto val) { return val * 2; };

    std::vector<int> result = transform_to_vector(vec, func);

    ASSERT_THAT(result, testing::ElementsAre(2, 4, 6));
}

TEST(Algorithm, TransformToVectorChangeType) {
    auto vec = std::vector<int> {1, 2, 3};
    auto func = [](auto val) -> double { return val + 0.5; };

    std::vector<double> result = transform_to_vector(vec, func);

    ASSERT_THAT(result, testing::ElementsAre(1.5, 2.5, 3.5));
}

struct MemberTest {
    int val {};

    auto proj_times_three() -> int { return val * 3; }
};

TEST(Algorithm, TransformToVectorMemberFunction) {
    auto vec = std::vector<MemberTest> {MemberTest {1}, MemberTest {2}, MemberTest {3}};

    std::vector<int> result = transform_to_vector(vec, &MemberTest::proj_times_three);

    ASSERT_THAT(result, testing::ElementsAre(3, 6, 9));
}

}  // namespace logicsim
