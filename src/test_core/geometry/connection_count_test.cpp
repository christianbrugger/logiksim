#include "core/geometry/connection_count.h"

#include "core/algorithm/to_vector.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

TEST(GeometryConnectionCount, FirstLastId) {
    const auto id = connection_id_t {10};
    const auto count = connection_count_t {std::size_t {id} + 1};

    EXPECT_EQ(first_id(count), connection_id_t {0});
    EXPECT_EQ(last_id(count), id);
}

TEST(GeometryConnectionCount, IdRange) {
    const auto r = id_range(connection_count_t {8});

    ASSERT_THAT(
        to_vector(r),
        testing::ElementsAre(connection_id_t(0), connection_id_t(1), connection_id_t(2),
                             connection_id_t(3), connection_id_t(4), connection_id_t(5),
                             connection_id_t(6), connection_id_t(7)));
}

}  // namespace logicsim
