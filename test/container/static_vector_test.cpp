
#include "container/static_vector.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>

namespace logicsim {

TEST(ContainerStaticVector, Construction) {
    const auto buffer = static_vector<int, 2> {};

    ASSERT_EQ(buffer.capacity(), 2);
    ASSERT_EQ(buffer.max_size(), 2);
    ASSERT_EQ(buffer.size(), 0);
    ASSERT_EQ(buffer.empty(), true);
}

TEST(ContainerStaticVector, ConstructionCount) {
    const auto buffer = static_vector<int, 4>(2);

    ASSERT_EQ(buffer.capacity(), 4);
    ASSERT_EQ(buffer.max_size(), 4);
    ASSERT_EQ(buffer.size(), 2);
    ASSERT_EQ(buffer.empty(), false);

    ASSERT_EQ(buffer[0], 0);
    ASSERT_EQ(buffer[1], 0);
}

TEST(ContainerStaticVector, ConstructionCountValue) {
    const auto buffer = static_vector<int, 4>(2, 42);

    ASSERT_EQ(buffer.capacity(), 4);
    ASSERT_EQ(buffer.max_size(), 4);
    ASSERT_EQ(buffer.size(), 2);
    ASSERT_EQ(buffer.empty(), false);

    ASSERT_EQ(buffer[0], 42);
    ASSERT_EQ(buffer[1], 42);
}

TEST(ContainerStaticVector, ConstructionListInitializer) {
    const auto buffer = static_vector<int, 4> {1, 2, 3};

    ASSERT_EQ(buffer.capacity(), 4);
    ASSERT_EQ(buffer.max_size(), 4);
    ASSERT_EQ(buffer.size(), 3);
    ASSERT_EQ(buffer.empty(), false);

    ASSERT_EQ(buffer[0], 1);
    ASSERT_EQ(buffer[1], 2);
    ASSERT_EQ(buffer[2], 3);
}

TEST(ContainerStaticVector, ComparisonEqual) {
    {
        auto buffer1 = static_vector<int, 4> {1, 2, 3};
        auto buffer2 = static_vector<int, 4> {1, 2, 3};
        ASSERT_EQ(buffer1 == buffer2, true);
    }
    {
        auto buffer1 = static_vector<int, 4> {1, 2, 3};
        auto buffer2 = static_vector<int, 4> {1, 2, 4};
        ASSERT_EQ(buffer1 == buffer2, false);
    }
    {
        auto buffer1 = static_vector<int, 4> {1, 2, 3};
        auto buffer2 = static_vector<int, 4> {1, 2, 3, 4};
        ASSERT_EQ(buffer1 == buffer2, false);
    }
    // different type & size
    {
        const auto buffer1 = static_vector<int, 4> {1, 2, 3};
        const auto buffer2 = static_vector<short, 3> {1, 2, 3};
        ASSERT_EQ(buffer1 == buffer2, true);
    }
    {
        const auto buffer1 = static_vector<int, 4> {1, 2, 3};
        const auto buffer2 = static_vector<short, 3> {1, 2, 4};
        ASSERT_EQ(buffer1 == buffer2, false);
    }
    {
        const auto buffer1 = static_vector<int, 4> {1, 2, 3, 4};
        const auto buffer2 = static_vector<short, 3> {1, 2, 3};
        ASSERT_EQ(buffer1 == buffer2, false);
    }
}

TEST(ContainerStaticVector, ComparisonThreeWay) {
    {
        auto buffer1 = static_vector<int, 4> {1, 1, 1};
        auto buffer2 = static_vector<int, 3> {1, 2, 3};

        ASSERT_EQ(buffer1 < buffer2, true);
        ASSERT_EQ(buffer1 <= buffer2, true);

        ASSERT_EQ(buffer1 > buffer2, false);
        ASSERT_EQ(buffer1 >= buffer2, false);
    }
    {
        const auto buffer1 = static_vector<int, 4> {1, 1, 1};
        const auto buffer2 = static_vector<short, 3> {1, 2};

        ASSERT_EQ(buffer1 < buffer2, true);
        ASSERT_EQ(buffer1 <= buffer2, true);

        ASSERT_EQ(buffer1 > buffer2, false);
        ASSERT_EQ(buffer1 >= buffer2, false);
    }
}

TEST(ContainerStaticVector, SizeOf) {
    ASSERT_EQ(sizeof(static_vector<uint32_t, 2, uint32_t>), 3 * sizeof(uint32_t));

    ASSERT_EQ(sizeof(static_vector<uint32_t, 4, uint8_t>), 5 * sizeof(uint32_t));

    ASSERT_EQ(sizeof(static_vector<uint8_t, 1, uint32_t>), 2 * sizeof(uint32_t));
    ASSERT_EQ(sizeof(static_vector<uint8_t, 4, uint32_t>), 2 * sizeof(uint32_t));

    // compilers don't support yet [[no_unique_address]]
    // ASSERT_EQ(sizeof(static_vector<uint32_t, 0, uint32_t>), 1 * sizeof(uint32_t));
}

TEST(ContainerStaticVector, Modify) {
    auto buffer = static_vector<int, 10> {1, 2};

    ASSERT_EQ(buffer.size(), 2);
    ASSERT_EQ(buffer.at(0), 1);
    ASSERT_EQ(buffer.at(1), 2);

    buffer.push_back(10);

    ASSERT_EQ(buffer.size(), 3);
    ASSERT_EQ(buffer.at(0), 1);
    ASSERT_EQ(buffer.at(1), 2);
    ASSERT_EQ(buffer.at(2), 10);

    buffer.pop_back();

    ASSERT_EQ(buffer.size(), 2);
    ASSERT_EQ(buffer.at(0), 1);
    ASSERT_EQ(buffer.at(1), 2);

    buffer.clear();

    ASSERT_EQ(buffer.size(), 0);
}

TEST(ContainerStaticVector, Exception) {
    // constructor
    EXPECT_THROW(static_cast<void>(static_vector<int, 2>(10)), std::exception);
    EXPECT_THROW(static_cast<void>(static_vector<int, 2>(10, 5)), std::exception);
    EXPECT_THROW(static_cast<void>(static_vector<int, 2> {1, 2, 3}), std::exception);

    // at
    {
        auto buffer = static_vector<int, 2> {1, 2};
        EXPECT_THROW(static_cast<void>(buffer.at(2)), std::exception);
    }
    {
        const auto buffer = static_vector<int, 4> {1, 2};
        EXPECT_THROW(static_cast<void>(buffer.at(2)), std::exception);
    }

    // push
    {
        auto buffer = static_vector<int, 3> {1, 2, 3};
        EXPECT_THROW(buffer.push_back(10), std::exception);
    }

    // pop
    {
        auto buffer = static_vector<int, 3> {};
        EXPECT_THROW(buffer.pop_back(), std::exception);
    }
}

TEST(ContainerStaticVector, ConstexprConstruction) {
    {
        constexpr static auto buffer = static_vector<int, 4>(2, 10);
        constexpr static auto size = buffer.size();
        ASSERT_EQ(size, 2);
    }
    {
        constexpr static auto buffer = static_vector<int, 4>(4, 10);
        constexpr static auto size = buffer.size();
        ASSERT_EQ(size, 4);
    }
    {
        constexpr static auto buffer = static_vector<int, 4> {1, 2, 3};
        constexpr static auto size = buffer.size();
        ASSERT_EQ(size, 3);
    }
}

}  // namespace logicsim