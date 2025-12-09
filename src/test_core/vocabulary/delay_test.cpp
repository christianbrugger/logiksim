#include "core/vocabulary/delay.h"

#include <gtest/gtest.h>

namespace logicsim {

TEST(VocabularyDelay, Overflow) {
    // count
    EXPECT_EQ(delay_t {100ns}.count_ns(), 100);
    EXPECT_EQ(delay_t {1us}.count_ns(), 1'000);

    // comparison
    EXPECT_EQ(delay_t {1000ns} == delay_t {1us}, true);
    EXPECT_EQ(delay_t {100ns} > delay_t {-10ns}, true);
    EXPECT_EQ(delay_t {100ns} <= delay_t {-10ns}, false);

    // epsilon, zero
    EXPECT_EQ(delay_t::zero().count_ns(), 0);
    EXPECT_EQ(delay_t::epsilon().count_ns(), 1);

    // add
    EXPECT_EQ(delay_t {100ns} + delay_t {10ns}, delay_t {110ns});
    {
        auto delay = delay_t {100ns};
        delay += delay_t {10ns};
        EXPECT_EQ(delay, delay_t {110ns});
    }
    EXPECT_THROW(static_cast<void>(delay_t::max() + delay_t::epsilon()), std::exception);

    // substract
    EXPECT_EQ(delay_t {100ns} - delay_t {10ns}, delay_t {90ns});
    {
        auto delay = delay_t {100ns};
        delay -= delay_t {10ns};
        EXPECT_EQ(delay, delay_t {90ns});
    }
    EXPECT_THROW(static_cast<void>(delay_t::min() - delay_t::epsilon()), std::exception);

    // multiply
    EXPECT_EQ(delay_t {100ns} * 2, delay_t {200ns});
    EXPECT_EQ(2 * delay_t {100ns}, delay_t {200ns});
    {
        auto delay = delay_t {100ns};
        delay *= 3;
        EXPECT_EQ(delay, delay_t {300ns});
    }
    EXPECT_THROW(static_cast<void>(delay_t::max() * 2), std::exception);

    // divide
    EXPECT_EQ(delay_t {100ns} / 2, delay_t {50ns});
    {
        auto delay = delay_t {100ns};
        delay /= 3;
        EXPECT_EQ(delay, delay_t {33ns});
    }
    EXPECT_THROW(static_cast<void>(delay_t::min() / -1), std::exception);
    EXPECT_THROW(static_cast<void>(delay_t {10ns} / 0), std::exception);

    // unary
    EXPECT_EQ(+delay_t {100ns}, delay_t {100ns});
    EXPECT_EQ(-delay_t {100ns}, delay_t {-100ns});
    EXPECT_EQ(-delay_t {-100ns}, delay_t {100ns});
    EXPECT_THROW(static_cast<void>(-delay_t::min()), std::exception);
}

}  // namespace logicsim
