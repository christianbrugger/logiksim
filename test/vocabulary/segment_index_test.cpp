#include "vocabulary/segment_index.h"

#include <gtest/gtest.h>

namespace logicsim {

TEST(VocabularySegmentIndex, Overflow) {
    // bool
    EXPECT_EQ(bool {null_segment_index}, false);
    EXPECT_EQ(bool {segment_index_t {10}}, true);

    // size_t
    EXPECT_EQ(std::size_t {segment_index_t {10}}, std::size_t {10});
    EXPECT_THROW(std::size_t {null_segment_index}, std::runtime_error);

    // comparison
    EXPECT_EQ(segment_index_t {10} < segment_index_t {11}, true);
    EXPECT_EQ(segment_index_t {10} >= segment_index_t {11}, false);

    // increment
    EXPECT_EQ(++segment_index_t {10}, segment_index_t {11});
    EXPECT_THROW(++segment_index_t::max(), std::runtime_error);
    EXPECT_THROW(++segment_index_t {null_segment_index}, std::runtime_error);
    {
        auto id = segment_index_t {10};
        EXPECT_EQ(id++, segment_index_t {10});
        EXPECT_EQ(id, segment_index_t {11});
    }

    // decrement
    EXPECT_EQ(--segment_index_t {10}, segment_index_t {9});
    EXPECT_THROW(--segment_index_t {0}, std::runtime_error);
    EXPECT_THROW(--segment_index_t {null_segment_index}, std::runtime_error);
    {
        auto id = segment_index_t {10};
        EXPECT_EQ(id--, segment_index_t {10});
        EXPECT_EQ(id, segment_index_t {9});
    }
}

}  // namespace logicsim
