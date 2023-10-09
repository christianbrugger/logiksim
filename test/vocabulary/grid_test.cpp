#include "vocabulary/grid.h"

#include <gtest/gtest.h>

namespace logicsim {

TEST(VocabularyGrid, Overflow) {
    // constructor
    EXPECT_THROW(static_cast<void>(grid_t {int {grid_t::max()} + int {1}}),
                 std::exception);
    EXPECT_THROW(static_cast<void>(grid_t {int {grid_t::min()} - int {1}}),
                 std::exception);

    // int
    EXPECT_EQ(int {grid_t {100}}, 100);
    EXPECT_EQ(int {grid_t {-50}}, -50);

    // comparison
    EXPECT_EQ(grid_t {10} == grid_t {10}, true);
    EXPECT_EQ(grid_t {-10} < grid_t {10}, true);
    EXPECT_EQ(grid_t {-10} >= grid_t {10}, false);

    // increment
    EXPECT_EQ(++grid_t {10}, grid_t {11});
    EXPECT_THROW(++grid_t::max(), std::exception);
    {
        auto count = grid_t {10};
        EXPECT_EQ(count++, grid_t {10});
        EXPECT_EQ(count, grid_t {11});
    }

    // decrement
    EXPECT_EQ(--grid_t {10}, grid_t {9});
    EXPECT_THROW(--grid_t::min(), std::exception);
    {
        auto count = grid_t {10};
        EXPECT_EQ(count--, grid_t {10});
        EXPECT_EQ(count, grid_t {9});
    }

    // add
    EXPECT_EQ(grid_t {100} + grid_t {10}, grid_t {110});
    {
        auto grid = grid_t {100};
        grid += grid_t {10};
        EXPECT_EQ(grid, grid_t {110});
    }
    EXPECT_THROW(static_cast<void>(grid_t::max() + grid_t {1}), std::exception);

    // substract
    EXPECT_EQ(grid_t {100} - grid_t {10}, grid_t {90});
    {
        auto grid = grid_t {100};
        grid -= grid_t {10};
        EXPECT_EQ(grid, grid_t {90});
    }
    EXPECT_THROW(static_cast<void>(grid_t::min() - grid_t {1}), std::exception);

    // multiply
    EXPECT_EQ(grid_t {100} * 2, grid_t {200});
    EXPECT_EQ(2 * grid_t {100}, grid_t {200});
    {
        auto grid = grid_t {100};
        grid *= 3;
        EXPECT_EQ(grid, grid_t {300});
    }
    EXPECT_THROW(static_cast<void>(grid_t::max() * 2), std::exception);

    // divide
    EXPECT_EQ(grid_t {100} / 2, grid_t {50});
    {
        auto grid = grid_t {100};
        grid /= 3;
        EXPECT_EQ(grid, grid_t {33});
    }
    EXPECT_THROW(static_cast<void>(grid_t::min() / -1), std::exception);
    EXPECT_THROW(static_cast<void>(grid_t {10} / 0), std::runtime_error);

    // unary
    EXPECT_EQ(+grid_t {100}, grid_t {100});
    EXPECT_EQ(-grid_t {100}, grid_t {-100});
    EXPECT_EQ(-grid_t {-100}, grid_t {100});
    EXPECT_THROW(static_cast<void>(-grid_t::min()), std::exception);
}

}  // namespace logicsim
