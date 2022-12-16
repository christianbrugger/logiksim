/// Make sure that our random numbers are deterministic.

/// Our benchmark creates random circuits and events. For the benchmark
/// to make sense these need to be deterministic on all platforms.
/// We test here the random numbers are still the same.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <fmt/ranges.h>
#include <range/v3/all.hpp>

TEST(Random, GeneratorStability) {
    boost::random::mt19937 rng;
    rng.seed(0);

    ASSERT_EQ(rng(), 2357136044);
    ASSERT_EQ(rng(), 2546248239);
    ASSERT_EQ(rng(), 3071714933);
    ASSERT_EQ(rng(), 3626093760);
}

TEST(Random, UniformInt_Stability_int32) {
    boost::random::mt19937 rng;
    rng.seed(0);

    boost::random::uniform_int_distribution<int32_t> numbers(0, 1'000'000);

    ASSERT_EQ(numbers(rng), 548937);
    ASSERT_EQ(numbers(rng), 592978);
    ASSERT_EQ(numbers(rng), 715350);
    ASSERT_EQ(numbers(rng), 844455);
}

TEST(Random, UniformInt_Stability_int64) {
    boost::random::mt19937 rng;
    rng.seed(0);

    boost::random::uniform_int_distribution<int64_t> numbers(10'000'000'000,
                                                             20'000'000'000);

    ASSERT_EQ(numbers(rng), 16652103340);
    ASSERT_EQ(numbers(rng), 16114550793);
    ASSERT_EQ(numbers(rng), 17069061397);
    ASSERT_EQ(numbers(rng), 11879422756);
}

TEST(Random, UniformInt_Stability_int8) {
    boost::random::mt19937 rng;
    rng.seed(0);

    boost::random::uniform_int_distribution<int8_t> numbers(0, 100);

    ASSERT_EQ(numbers(rng), 55);
    ASSERT_EQ(numbers(rng), 59);
    ASSERT_EQ(numbers(rng), 72);
    ASSERT_EQ(numbers(rng), 85);
}

TEST(Random, Shuffle_Stability) {
    boost::random::mt19937 rng;
    rng.seed(0);

    std::vector<int> vec = ranges::views::iota(0, 10) | ranges::to_vector;

    vec |= ranges::actions::shuffle(rng);
    ASSERT_THAT(vec, testing::ElementsAre(0, 1, 9, 2, 7, 4, 5, 8, 6, 3));

    vec |= ranges::actions::shuffle(rng);
    ASSERT_THAT(vec, testing::ElementsAre(9, 3, 4, 7, 5, 1, 0, 8, 6, 2));
}