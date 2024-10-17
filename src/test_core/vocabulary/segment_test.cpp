#include "core/vocabulary/segment.h"

#include <gtest/gtest.h>

#include <ankerl/unordered_dense.h>

namespace logicsim {

TEST(VocabularySegment, Overflow) {
    // constructor
    EXPECT_THROW(static_cast<void>(segment_t {wire_id_t {1}, segment_index_t {-1}}),
                 std::exception);
    EXPECT_THROW(static_cast<void>(segment_t {wire_id_t {-1}, segment_index_t {1}}),
                 std::exception);

    // comparison
    {
        const auto segment1 = segment_t {wire_id_t {1}, segment_index_t {1}};
        const auto segment2 = segment_t {wire_id_t {1}, segment_index_t {1}};
        EXPECT_TRUE(segment1 == segment2);
    }

    // bool
    EXPECT_EQ(bool {segment_t(wire_id_t(0), segment_index_t(0))}, true);
    EXPECT_EQ(bool {null_segment}, false);
}

TEST(VocabularySegment, Hashing) {
    const auto hash = ankerl::unordered_dense::hash<segment_t> {};

    EXPECT_TRUE(hash(segment_t(wire_id_t(1), segment_index_t(0))) !=
                hash(segment_t(wire_id_t(0), segment_index_t(0))));

    EXPECT_TRUE(hash(segment_t(wire_id_t(1), segment_index_t(0))) !=
                hash(segment_t(wire_id_t(0), segment_index_t(1))));

    EXPECT_TRUE(hash(segment_t(wire_id_t(1), segment_index_t(1))) ==
                hash(segment_t(wire_id_t(1), segment_index_t(1))));

    // avalanching
    EXPECT_TRUE(hash(segment_t(wire_id_t(0), segment_index_t(1))) != 1);
    EXPECT_TRUE(hash(segment_t(wire_id_t(1), segment_index_t(0))) != 1);
}

}  // namespace logicsim
