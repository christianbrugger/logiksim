/// Make sure that our shuffle is deterministic.

#include "algorithm/shuffle.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/random/mersenne_twister.hpp>

#include <numeric>
#include <vector>

namespace logicsim {

TEST(Random, ShuffleStabilityIterators) {
    boost::random::mt19937 rng {0};

    std::vector<int> vec(10);
    std::iota(vec.begin(), vec.end(), 0);

    shuffle(std::begin(vec), std::end(vec), rng);
    ASSERT_THAT(vec, testing::ElementsAre(0, 2, 3, 5, 9, 1, 6, 8, 4, 7));

    shuffle(std::begin(vec), std::end(vec), rng);
    ASSERT_THAT(vec, testing::ElementsAre(8, 1, 7, 3, 2, 5, 6, 0, 4, 9));
}

TEST(Random, ShuffleStabilityRanges) {
    boost::random::mt19937 rng {0};

    std::vector<int> vec(10);
    std::iota(vec.begin(), vec.end(), 0);

    shuffle(vec, rng);
    ASSERT_THAT(vec, testing::ElementsAre(0, 2, 3, 5, 9, 1, 6, 8, 4, 7));

    shuffle(vec, rng);
    ASSERT_THAT(vec, testing::ElementsAre(8, 1, 7, 3, 2, 5, 6, 0, 4, 9));
}

}  // namespace logicsim