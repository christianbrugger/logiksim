/// Make sure that our random numbers are deterministic.

/// Our benchmark creates random circuits and events. For the benchmark
/// to make sense these need to be deterministic on all platforms.
/// We test here that random numbers are still the same.

#include "random/generator.h"

#include <gtest/gtest.h>

namespace logicsim {

TEST(RandomGenerator, StabilityClass) {
    auto rng = Rng {0};

    ASSERT_EQ(rng(), 2357136044);
    ASSERT_EQ(rng(), 2546248239);
    ASSERT_EQ(rng(), 3071714933);
    ASSERT_EQ(rng(), 3626093760);
}

TEST(RandomGenerator, StabilityMethod) {
    auto rng = get_random_number_generator(0);

    ASSERT_EQ(rng(), 2357136044);
    ASSERT_EQ(rng(), 2546248239);
    ASSERT_EQ(rng(), 3071714933);
    ASSERT_EQ(rng(), 3626093760);
}

}  // namespace logicsim
