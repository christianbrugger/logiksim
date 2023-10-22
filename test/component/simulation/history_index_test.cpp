
#include "component/simulation/history_index.h"

#include <gtest/gtest.h>

#include <cstdint>

namespace logicsim {

namespace simulation {

TEST(SimulationHistoryIndex, Comparisons) {
    EXPECT_EQ(history_index_t {0} == history_index_t {0}, true);
    EXPECT_EQ(history_index_t {0} == history_index_t {1}, false);

    EXPECT_EQ(history_index_t {0} < history_index_t {0}, false);
    EXPECT_EQ(history_index_t {0} < history_index_t {1}, true);

    EXPECT_EQ(history_index_t {100} < history_index_t::max(), true);
    EXPECT_EQ(history_index_t {-10} > history_index_t::min(), true);
}

TEST(SimulationHistoryIndex, Conversions) {
    // ptrdiff_t
    EXPECT_EQ(std::ptrdiff_t {history_index_t {-10}} == std::ptrdiff_t {-10}, true);
    EXPECT_EQ(std::ptrdiff_t {history_index_t {0}} == std::ptrdiff_t {0}, true);
    EXPECT_EQ(std::ptrdiff_t {history_index_t {10}} == std::ptrdiff_t {10}, true);
}

TEST(SimulationHistoryIndex, OperatorsNormal) {
    // increment
    {
        auto index = history_index_t {0};
        ++index;
        EXPECT_EQ(index == history_index_t {1}, true);
    }
    {
        auto index = history_index_t {0};
        EXPECT_EQ(index++ == history_index_t {0}, true);
        EXPECT_EQ(index == history_index_t {1}, true);
    }

    // history_index_t += integral
    {
        auto index = history_index_t {5};
        index += 10;
        EXPECT_EQ(index == history_index_t {15}, true);
    }
    {
        auto index = history_index_t {10};
        index += -5;
        EXPECT_EQ(index == history_index_t {5}, true);
    }

    // history_index_t -= integral
    {
        auto index = history_index_t {200};
        index -= int8_t {100};
        EXPECT_EQ(index == history_index_t {100}, true);
    }
    {
        auto index = history_index_t {200};
        index -= 50;
        EXPECT_EQ(index == history_index_t {150}, true);
    }

    // history_index_t - history_index_t
    EXPECT_EQ(history_index_t {10} - history_index_t {5} == std::ptrdiff_t {5}, true);
    EXPECT_EQ(history_index_t {5} - history_index_t {5} == std::ptrdiff_t {0}, true);

    // history_index_t + integral
    EXPECT_EQ(history_index_t {0} + 12 == history_index_t {12}, true);
    EXPECT_EQ(history_index_t {10} + 2 == history_index_t {12}, true);
    EXPECT_EQ(history_index_t {10} + std::ptrdiff_t {-2} == history_index_t {8}, true);

    // history_index_t - integral
    EXPECT_EQ(history_index_t {100} - 12 == history_index_t {88}, true);
    EXPECT_EQ(history_index_t {100} - 2 == history_index_t {98}, true);
    EXPECT_EQ(history_index_t {100} - std::ptrdiff_t {-2} == history_index_t {102}, true);

    // integral + history_index_t
    EXPECT_EQ(12 + history_index_t {0} == history_index_t {12}, true);
    EXPECT_EQ(2 + history_index_t {10} == history_index_t {12}, true);
    EXPECT_EQ(std::ptrdiff_t {-2} + history_index_t {10} == history_index_t {8}, true);
}

TEST(SimulationHistoryIndex, OperatorsOverflow) {
    const auto value_max = history_index_t::max().value;
    const auto value_min = history_index_t::min().value;

    // increment
    EXPECT_THROW(++history_index_t::max(), std::exception);
    EXPECT_THROW(static_cast<void>(history_index_t::max()++), std::exception);

    // history_index_t += integral
    {
        auto index = history_index_t {5};
        EXPECT_THROW(index += value_max, std::exception);
    }
    {
        auto index = history_index_t {-5};
        EXPECT_THROW(index += value_min, std::exception);
    }

    // history_index_t -= integral
    {
        auto index = history_index_t {-100};
        EXPECT_THROW(index -= value_max, std::exception);
    }
    {
        auto index = history_index_t::min();
        EXPECT_THROW(index -= 1, std::exception);
    }
    {
        auto index = history_index_t::max();
        EXPECT_THROW(index -= -1, std::exception);
    }

    // history_index_t - history_index_t
    EXPECT_THROW(static_cast<void>(history_index_t::max() - history_index_t::min()),
                 std::exception);
    EXPECT_THROW(static_cast<void>(history_index_t::min() - history_index_t::max()),
                 std::exception);

    // history_index_t + integral
    EXPECT_THROW(static_cast<void>(history_index_t::max() + 1), std::exception);
    EXPECT_THROW(static_cast<void>(history_index_t {10} + value_max), std::exception);
    EXPECT_THROW(static_cast<void>(history_index_t::min() + (-2)), std::exception);

    // history_index_t - integral
    EXPECT_THROW(static_cast<void>(history_index_t {10} - value_min), std::exception);
    EXPECT_THROW(static_cast<void>(history_index_t::min() - 100), std::exception);

    // integral + history_index_t
    EXPECT_THROW(static_cast<void>(value_max + history_index_t {1}), std::exception);
    EXPECT_THROW(static_cast<void>(1 + history_index_t::max()), std::exception);
    EXPECT_THROW(static_cast<void>(value_min + history_index_t {-1}),
                 std::exception);
}

}  // namespace simulation

}  // namespace logicsim
