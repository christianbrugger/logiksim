#include "vocabulary/connection_id.h"

#include <gtest/gtest.h>

namespace logicsim {

TEST(VocabularyConnectionId, Overflow) {
    // bool
    EXPECT_EQ(bool {null_connection_id}, false);
    EXPECT_EQ(bool {connection_id_t {10}}, true);

    // std::size_t
    EXPECT_EQ(std::size_t {connection_id_t {10}}, std::size_t {10});
    EXPECT_THROW(static_cast<void>(std::size_t {null_connection_id}), std::runtime_error);

    // comparison
    EXPECT_EQ(connection_id_t {10} < connection_id_t {11}, true);
    EXPECT_EQ(connection_id_t {10} >= connection_id_t {11}, false);

    // increment
    EXPECT_EQ(++connection_id_t {10}, connection_id_t {11});
    EXPECT_THROW(++connection_id_t::max(), std::runtime_error);
    EXPECT_THROW(++connection_id_t {null_connection_id}, std::runtime_error);
    {
        auto id = connection_id_t {10};
        EXPECT_EQ(id++, connection_id_t {10});
        EXPECT_EQ(id, connection_id_t {11});
    }
}

}  // namespace logicsim
