#include "vocabulary/connection_id.h"

#include <gtest/gtest.h>

namespace logicsim {

TEST(VocabularyConnectionId, Overflow) {
    // bool
    EXPECT_EQ(bool {null_connection}, false);
    EXPECT_EQ(bool {connection_id_t {10}}, true);

    // size_t
    EXPECT_EQ(std::size_t {connection_id_t {10}}, std::size_t {10});
    EXPECT_THROW(std::size_t {null_connection}, std::runtime_error);

    // comparison
    EXPECT_EQ(connection_id_t {10} < connection_id_t {11}, true);
    EXPECT_EQ(connection_id_t {10} >= connection_id_t {11}, false);

    // increment
    EXPECT_EQ(++connection_id_t {10}, connection_id_t {11});
    EXPECT_THROW(++connection_id_t::max(), std::runtime_error);
    EXPECT_THROW(++connection_id_t {null_connection}, std::runtime_error);
    {
        auto id = connection_id_t {10};
        EXPECT_EQ(id++, connection_id_t {10});
        EXPECT_EQ(id, connection_id_t {11});
    }
}

}  // namespace logicsim
