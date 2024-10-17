#include "core/vocabulary/offset.h"

#include <gtest/gtest.h>

namespace logicsim {

TEST(VocabularyOffset, Overflow) {
    // constructor
    EXPECT_THROW(static_cast<void>(offset_t {int {offset_t::max()} + int {1}}),
                 std::exception);
    EXPECT_THROW(static_cast<void>(offset_t {int {offset_t::min()} - int {1}}),
                 std::exception);

    // int
    EXPECT_EQ(int {offset_t {100}}, 100);
    EXPECT_EQ(int {offset_t {0}}, 0);

    // comparison
    EXPECT_EQ(offset_t {10} == offset_t {10}, true);
    EXPECT_EQ(offset_t {1} < offset_t {10}, true);
    EXPECT_EQ(offset_t {1} >= offset_t {10}, false);

    // increment
    EXPECT_EQ(++offset_t {10}, offset_t {11});
    EXPECT_THROW(++offset_t::max(), std::exception);
    {
        auto count = offset_t {10};
        EXPECT_EQ(count++, offset_t {10});
        EXPECT_EQ(count, offset_t {11});
    }

    // decrement
    EXPECT_EQ(--offset_t {10}, offset_t {9});
    EXPECT_THROW(--offset_t::min(), std::exception);
    {
        auto count = offset_t {10};
        EXPECT_EQ(count--, offset_t {10});
        EXPECT_EQ(count, offset_t {9});
    }

    // add
    EXPECT_EQ(offset_t {100} + offset_t {10}, offset_t {110});
    {
        auto offset = offset_t {100};
        offset += offset_t {10};
        EXPECT_EQ(offset, offset_t {110});
    }
    EXPECT_THROW(static_cast<void>(offset_t::max() + offset_t {1}), std::exception);

    // substract
    EXPECT_EQ(offset_t {100} - offset_t {10}, offset_t {90});
    {
        auto offset = offset_t {100};
        offset -= offset_t {10};
        EXPECT_EQ(offset, offset_t {90});
    }
    EXPECT_THROW(static_cast<void>(offset_t::min() - offset_t {1}), std::exception);

    // multiply
    EXPECT_EQ(offset_t {100} * 2, offset_t {200});
    EXPECT_EQ(2 * offset_t {100}, offset_t {200});
    {
        auto offset = offset_t {100};
        offset *= 3;
        EXPECT_EQ(offset, offset_t {300});
    }
    EXPECT_THROW(static_cast<void>(offset_t::max() * 2), std::exception);

    // divide
    EXPECT_EQ(offset_t {100} / 2, offset_t {50});
    {
        auto offset = offset_t {100};
        offset /= 3;
        EXPECT_EQ(offset, offset_t {33});
    }
    EXPECT_THROW(static_cast<void>(offset_t {10} / 0), std::exception);
}

}  // namespace logicsim
