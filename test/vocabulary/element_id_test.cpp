#include "vocabulary/element_id.h"

#include <gtest/gtest.h>

namespace logicsim {

TEST(VocabularyElementId, Overflow) {
    // bool
    EXPECT_EQ(bool {null_element}, false);
    EXPECT_EQ(bool {element_id_t {10}}, true);

    // size_t
    EXPECT_EQ(std::size_t {element_id_t {10}}, std::size_t {10});
    EXPECT_THROW(std::size_t {null_element}, std::runtime_error);

    // comparison
    EXPECT_EQ(element_id_t {10} < element_id_t {11}, true);
    EXPECT_EQ(element_id_t {10} >= element_id_t {11}, false);

    // increment
    EXPECT_EQ(++element_id_t {10}, element_id_t {11});
    EXPECT_THROW(++element_id_t::max(), std::runtime_error);
    EXPECT_THROW(++element_id_t {null_element}, std::runtime_error);
    {
        auto id = element_id_t {10};
        EXPECT_EQ(id++, element_id_t {10});
        EXPECT_EQ(id, element_id_t {11});
    }
}

}  // namespace logicsim
