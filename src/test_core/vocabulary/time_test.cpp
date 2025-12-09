#include "core/vocabulary/time.h"

#include <gtest/gtest.h>

namespace logicsim {

TEST(VocabularyTime, Overflow) {
    // comparison
    EXPECT_EQ(time_t {1000ns} == time_t {1us}, true);
    EXPECT_EQ(time_t {100ns} > time_t {-10ns}, true);
    EXPECT_EQ(time_t {100ns} <= time_t {-10ns}, false);
    EXPECT_EQ(time_t::zero() < time_t::max(), true);

    // epsilon, zero
    EXPECT_EQ(time_t::epsilon().count_ns(), 1);

    // substract
    EXPECT_EQ(time_t {100ns} - time_t {10ns}, delay_t {90ns});
    EXPECT_EQ(time_t {10ns} - time_t {100ns}, delay_t {-90ns});

    EXPECT_THROW(static_cast<void>(time_t::max() - time_t::min()), std::exception);
    EXPECT_THROW(static_cast<void>(time_t::min() - time_t::max()), std::exception);
}

TEST(VocabularyTime, OperatorDelay) {
    // add
    EXPECT_EQ(time_t {100ns} + delay_t {10ns}, time_t {110ns});
    EXPECT_EQ(delay_t {100ns} + time_t {10ns}, time_t {110ns});
    {
        auto delay = time_t {100ns};
        delay += delay_t {10ns};
        EXPECT_EQ(delay, time_t {110ns});
    }
    EXPECT_THROW(static_cast<void>(time_t::max() + time_t::epsilon()), std::exception);

    // substract
    EXPECT_EQ(time_t {100ns} - delay_t {10ns}, time_t {90ns});
    {
        auto delay = time_t {100ns};
        delay -= delay_t {10ns};
        EXPECT_EQ(delay, time_t {90ns});
    }
    EXPECT_THROW(static_cast<void>(time_t::min() - time_t::epsilon()), std::exception);
}

}  // namespace logicsim
