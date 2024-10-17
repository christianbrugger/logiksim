#include "core/vocabulary/grid_fine.h"

#include <gtest/gtest.h>

#include <cmath>

namespace logicsim {

TEST(VocabularyGridFine, Overflow) {
    // constructor
    EXPECT_EQ(double {grid_fine_t {grid_t {100}}}, 100.);

    // double
    EXPECT_EQ(double {grid_fine_t {100}}, 100.);
    EXPECT_EQ(double {grid_fine_t {-50}}, -50.);

    // comparison
    EXPECT_EQ(grid_fine_t {10} == grid_fine_t {10}, true);
    EXPECT_EQ(grid_fine_t {-10} < grid_fine_t {10}, true);
    EXPECT_EQ(grid_fine_t {-10} >= grid_fine_t {10}, false);

    EXPECT_EQ(grid_fine_t {10} == grid_t {10}, true);
    EXPECT_EQ(grid_fine_t {-10} < grid_t {10}, true);
    EXPECT_EQ(grid_fine_t {-10} >= grid_t {10}, false);

    EXPECT_EQ(grid_t {10} == grid_fine_t {10}, true);
    EXPECT_EQ(grid_t {-10} < grid_fine_t {10}, true);
    EXPECT_EQ(grid_t {-10} >= grid_fine_t {10}, false);
}

TEST(VocabularyGridFine, OperatorsGridFine) {
    // grid_fine_t - add
    EXPECT_EQ(grid_fine_t {100} + grid_fine_t {10}, grid_fine_t {110});
    {
        auto grid = grid_fine_t {100};
        grid += grid_fine_t {10};
        EXPECT_EQ(grid, grid_fine_t {110});
    }

    // grid_fine_t - substract
    EXPECT_EQ(grid_fine_t {100} - grid_fine_t {10}, grid_fine_t {90});
    {
        auto grid = grid_fine_t {100};
        grid -= grid_fine_t {10};
        EXPECT_EQ(grid, grid_fine_t {90});
    }

    // grid_fine_t - multiply
    EXPECT_EQ(grid_fine_t {100} * 2, grid_fine_t {200});
    EXPECT_EQ(2 * grid_fine_t {100}, grid_fine_t {200});
    {
        auto grid = grid_fine_t {100};
        grid *= 3;
        EXPECT_EQ(grid, grid_fine_t {300});
    }

    // grid_fine_t - divide
    EXPECT_EQ(grid_fine_t {100} / 2, grid_fine_t {50});
    {
        auto grid = grid_fine_t {10};
        grid /= 4;
        EXPECT_EQ(grid, grid_fine_t {2.5});
    }
    EXPECT_THROW([] { return grid_fine_t {100} / 0; }(), std::exception);
}

TEST(VocabularyGridFine, OperatorsGrid) {
    // grid_t - add
    EXPECT_EQ(grid_t {100} + grid_fine_t {10}, grid_fine_t {110});
    EXPECT_EQ(grid_fine_t {100} + grid_t {10}, grid_fine_t {110});
    {
        auto grid = grid_fine_t {100};
        grid += grid_t {10};
        EXPECT_EQ(grid, grid_fine_t {110});
    }

    // grid_t - substract
    EXPECT_EQ(grid_t {100} - grid_fine_t {10}, grid_fine_t {90});
    EXPECT_EQ(grid_fine_t {100} - grid_t {10}, grid_fine_t {90});
    {
        auto grid = grid_fine_t {100};
        grid -= grid_t {10};
        EXPECT_EQ(grid, grid_fine_t {90});
    }

    // grid_t - multiply
    EXPECT_EQ(grid_t {100} * 2.5, grid_fine_t {250});
    EXPECT_EQ(2.5 * grid_t {100}, grid_fine_t {250});

    // grid_t - divide
    EXPECT_EQ(grid_t {10} / 2.5, grid_fine_t {4});
    EXPECT_THROW([] { return grid_t {100} / 0.0; }(), std::exception);
}

TEST(VocabularyGridFine, OperatorsUnary) {
    // unary
    EXPECT_EQ(+grid_fine_t {100}, grid_fine_t {100});
    EXPECT_EQ(-grid_fine_t {100}, grid_fine_t {-100});
    EXPECT_EQ(-grid_fine_t {-100}, grid_fine_t {100});
}

}  // namespace logicsim
