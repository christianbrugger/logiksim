/// Make sure that our random numbers are deterministic.

/// Our benchmark creates random circuits and events. For the benchmark
/// to make sense these need to be deterministic on all platforms.
/// We test here that random numbers are still the same.

#include "core/algorithm/uniform_int_distribution.h"

#include "core/random/generator.h"

#include <gtest/gtest.h>

#include <cstdint>

namespace logicsim {

TEST(AlgorithmUniformIntDistribution, StabilityInt32) {
    auto rng = Rng {0};

    const auto numbers = uint_distribution<int32_t>(0, 1'000'000);

    ASSERT_EQ(numbers(rng), 548937);
    ASSERT_EQ(numbers(rng), 592978);
    ASSERT_EQ(numbers(rng), 715350);
    ASSERT_EQ(numbers(rng), 844455);
}

TEST(AlgorithmUniformIntDistribution, StabilityInt64) {
    auto rng = Rng {0};

    const auto numbers = uint_distribution<int64_t>(10'000'000'000, 20'000'000'000);

    ASSERT_EQ(numbers(rng), 16652103340);
    ASSERT_EQ(numbers(rng), 16114550793);
    ASSERT_EQ(numbers(rng), 17069061397);
    ASSERT_EQ(numbers(rng), 11879422756);
}

TEST(AlgorithmUniformIntDistribution, StabilityInt8) {
    auto rng = Rng {0};

    const auto numbers = uint_distribution<int8_t>(0, 100);

    ASSERT_EQ(numbers(rng), 55);
    ASSERT_EQ(numbers(rng), 59);
    ASSERT_EQ(numbers(rng), 72);
    ASSERT_EQ(numbers(rng), 85);
}

}  // namespace logicsim
