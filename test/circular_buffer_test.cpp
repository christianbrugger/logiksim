
#include "circular_buffer.h"

#include "range.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fmt/core.h>

#include <cstdint>

namespace logicsim {

TEST(CircularBuffer, Construction) {
    circular_buffer<int32_t, 2, uint32_t> buffer {};

    std::vector<int> a;
    a.pop_back();
}

}  // namespace logicsim