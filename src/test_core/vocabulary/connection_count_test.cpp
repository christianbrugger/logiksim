#include "core/vocabulary/connection_count.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

TEST(VocabularyConnectionCount, Overflow) {
    // connstruction
    const auto max_id = connection_count_t {std::size_t {connection_id_t::max()}};
    const auto max_count = connection_count_t {std::size_t {max_id} + 1};
    EXPECT_EQ(max_count > max_id, true);
    EXPECT_THROW(static_cast<void>(connection_count_t {std::size_t {max_id} + 2}),
                 std::runtime_error);
    EXPECT_THROW(static_cast<void>(connection_count_t {-1}), std::runtime_error);

    // std::size_t
    EXPECT_EQ(std::size_t {connection_count_t {10}}, std::size_t {10});
    // count
    EXPECT_EQ(std::size_t {connection_count_t {10}.count()}, 10);

    // comparison
    EXPECT_EQ(connection_count_t {10} < connection_count_t {11}, true);
    EXPECT_EQ(connection_count_t {10} >= connection_count_t {11}, false);
    // comparison with id
    EXPECT_EQ(connection_count_t {10} < connection_id_t {11}, true);
    EXPECT_EQ(connection_count_t {10} >= connection_id_t {11}, false);

    // increment
    EXPECT_EQ(++connection_count_t {10}, connection_count_t {11});
    EXPECT_THROW(++connection_count_t::max(), std::runtime_error);
    {
        auto count = connection_count_t {10};
        EXPECT_EQ(count++, connection_count_t {10});
        EXPECT_EQ(count, connection_count_t {11});
    }

    // decrement
    EXPECT_EQ(--connection_count_t {10}, connection_count_t {9});
    EXPECT_THROW(--connection_count_t::min(), std::runtime_error);
    {
        auto count = connection_count_t {10};
        EXPECT_EQ(count--, connection_count_t {10});
        EXPECT_EQ(count, connection_count_t {9});
    }

    // add
    EXPECT_EQ(connection_count_t {10} + connection_count_t {11}, connection_count_t {21});
    EXPECT_THROW(static_cast<void>(connection_count_t::max() + connection_count_t {11}),
                 std::runtime_error);
    {
        auto count = connection_count_t {20};
        count += connection_count_t {5};
        EXPECT_EQ(count, connection_count_t {25});
    }

    // substract
    EXPECT_EQ(connection_count_t {11} - connection_count_t {10}, connection_count_t {1});
    EXPECT_THROW(static_cast<void>(connection_count_t::min() - connection_count_t {11}),
                 std::runtime_error);
    EXPECT_THROW(static_cast<void>(connection_count_t {11} - connection_count_t::max()),
                 std::runtime_error);
    EXPECT_EQ(connection_count_t::max() - connection_count_t::max(),
              connection_count_t {0});
    {
        auto count = connection_count_t {20};
        count -= connection_count_t {5};
        EXPECT_EQ(count, connection_count_t {15});
    }

    // multiply
    EXPECT_EQ(connection_count_t {11} * 2, connection_count_t {22});
    EXPECT_EQ(2 * connection_count_t {11}, connection_count_t {22});
    EXPECT_THROW(static_cast<void>(connection_count_t::max() * 2), std::runtime_error);
    {
        auto count = connection_count_t {20};
        count *= 3;
        EXPECT_EQ(count, connection_count_t {60});
    }
}

}  // namespace logicsim
