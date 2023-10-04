
#include "container/circular_buffer.h"

#include "range.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fmt/core.h>

#include <cstdint>

namespace logicsim {

TEST(CircularBuffer, Construction) {
    circular_buffer<int32_t, 2, uint32_t> buffer {};

    ASSERT_EQ(buffer.capacity(), 2);
    ASSERT_EQ(buffer.size(), 0);
}

TEST(CircularBuffer, PushBack) {
    circular_buffer<int32_t, 2, uint32_t> buffer {};

    buffer.push_back(1);
    ASSERT_EQ(buffer.capacity(), 2);
    ASSERT_EQ(buffer.size(), 1);

    buffer.push_back(2);
    ASSERT_EQ(buffer.capacity(), 2);
    ASSERT_EQ(buffer.size(), 2);

    ASSERT_EQ(buffer.at(0), 1);
    ASSERT_EQ(buffer.at(1), 2);

    ASSERT_EQ(buffer[0], 1);
    ASSERT_EQ(buffer[1], 2);
}

TEST(CircularBuffer, PopBackSimple) {
    circular_buffer<int32_t, 2, uint32_t> buffer {};

    buffer.push_back(1);
    buffer.push_back(2);
    buffer.pop_back();

    ASSERT_EQ(buffer.capacity(), 2);
    ASSERT_EQ(buffer.size(), 1);

    ASSERT_EQ(buffer[0], 1);
}

TEST(CircularBuffer, PopFrontSimple) {
    circular_buffer<int32_t, 2, uint32_t> buffer {};

    buffer.push_back(1);
    buffer.push_back(2);
    buffer.pop_front();

    ASSERT_EQ(buffer.capacity(), 2);
    ASSERT_EQ(buffer.size(), 1);

    ASSERT_EQ(buffer[0], 2);
}

TEST(CircularBuffer, PushBackSimpleGrowth) {
    circular_buffer<int32_t, 2, uint32_t> buffer {};

    buffer.push_back(1);
    buffer.push_back(2);
    buffer.push_back(3);

    ASSERT_EQ(buffer.capacity(), 4);
    ASSERT_EQ(buffer.size(), 3);

    ASSERT_EQ(buffer[0], 1);
    ASSERT_EQ(buffer[1], 2);
    ASSERT_EQ(buffer[2], 3);
}

TEST(CircularBuffer, PushBackComplexGrowth) {
    circular_buffer<int32_t, 2, uint32_t> buffer {};

    buffer.push_back(1);
    buffer.push_back(2);
    buffer.pop_front();
    buffer.push_back(3);
    buffer.push_back(4);

    ASSERT_EQ(buffer.size(), 3);
    ASSERT_EQ(buffer.capacity(), 4);

    ASSERT_EQ(buffer[0], 2);
    ASSERT_EQ(buffer[1], 3);
    ASSERT_EQ(buffer[2], 4);
}

TEST(CircularBuffer, PushFrontSimple) {
    circular_buffer<int32_t, 2, uint32_t> buffer {};

    buffer.push_front(1);
    buffer.push_front(2);
    buffer.push_front(3);
    buffer.push_front(4);

    ASSERT_EQ(buffer.size(), 4);
    ASSERT_EQ(buffer.capacity(), 4);

    ASSERT_EQ(buffer[0], 4);
    ASSERT_EQ(buffer[1], 3);
    ASSERT_EQ(buffer[2], 2);
    ASSERT_EQ(buffer[3], 1);
}

TEST(CircularBuffer, PushFrontComplex) {
    circular_buffer<int32_t, 2, uint32_t> buffer {};

    buffer.push_front(1);
    buffer.push_front(2);
    buffer.pop_back();
    buffer.push_front(3);
    buffer.push_front(4);

    ASSERT_EQ(buffer.size(), 3);
    ASSERT_EQ(buffer.capacity(), 4);

    ASSERT_EQ(buffer[0], 4);
    ASSERT_EQ(buffer[1], 3);
    ASSERT_EQ(buffer[2], 2);
}

TEST(CircularBuffer, TestPushFrontBack) {
    circular_buffer<int32_t, 2, uint32_t> buffer {};

    buffer.push_front(1);
    buffer.push_back(2);
    buffer.push_front(0);
    buffer.push_back(3);

    ASSERT_EQ(buffer.size(), 4);
    ASSERT_EQ(buffer.capacity(), 4);

    ASSERT_EQ(buffer[0], 0);
    ASSERT_EQ(buffer[1], 1);
    ASSERT_EQ(buffer[2], 2);
    ASSERT_EQ(buffer[3], 3);
}

TEST(CircularBuffer, TestClear) {
    circular_buffer<int32_t, 2, uint32_t> buffer {};

    buffer.push_front(1);
    buffer.push_back(2);
    buffer.clear();

    ASSERT_EQ(buffer.size(), 0);
    ASSERT_EQ(buffer.capacity(), 2);
}

TEST(CircularBuffer, AlmostFull) {
    circular_buffer<uint8_t, 2, uint8_t> buffer {};

    ASSERT_EQ(buffer.size(), 0);
    ASSERT_EQ(buffer.capacity(), 2);

    ASSERT_EQ(buffer.max_size(), 63);
    buffer.reserve(63);
    ASSERT_EQ(buffer.capacity(), 63);

    // set start pointer to the end
    buffer.push_front(0);
    buffer.pop_back();

    for (auto i : range<uint8_t>(63)) {
        buffer.push_back(i);
    }
    for (auto i : range<uint8_t>(63)) {
        ASSERT_EQ(buffer[i], i);
    }
}

TEST(CircularBuffer, TestIterators) {
    circular_buffer<int32_t, 2, uint32_t> buffer {};

    buffer.push_front(1);
    buffer.push_back(2);
    buffer.push_front(0);
    buffer.push_back(3);

    ASSERT_EQ(std::size(buffer), 4);
    ASSERT_EQ(std::distance(std::begin(buffer), std::end(buffer)), 4);
    ASSERT_THAT(buffer, testing::ElementsAre(0, 1, 2, 3));

    const auto &const_buffer = buffer;
    ASSERT_EQ(std::distance(std::begin(const_buffer), std::end(const_buffer)), 4);
    ASSERT_THAT(const_buffer, testing::ElementsAre(0, 1, 2, 3));
}

TEST(CircularBuffer, TestIteratorRanges) {
    circular_buffer<int32_t, 2, uint32_t> buffer {};

    buffer.push_front(1);
    buffer.push_back(2);
    buffer.push_front(0);
    buffer.push_back(3);

    ASSERT_EQ(std::ranges::distance(buffer), 4);
    const auto &const_buffer = buffer;
    ASSERT_EQ(std::ranges::distance(const_buffer), 4);
}

TEST(CircularBuffer, HasNonThrowingSwap) {
    circular_buffer<int32_t, 2, uint32_t> buffer1 {};
    circular_buffer<int32_t, 2, uint32_t> buffer2 {};

    static_assert(noexcept(std::swap(buffer1, buffer2)));
}

}  // namespace logicsim