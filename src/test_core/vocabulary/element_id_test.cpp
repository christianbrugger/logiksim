#include "core/vocabulary/element_id.h"

#include <gtest/gtest.h>

#include <ankerl/unordered_dense.h>

namespace logicsim {

TEST(VocabularyElementId, Overflow) {
    // bool
    EXPECT_EQ(bool {null_element}, false);
    EXPECT_EQ(bool {element_id_t {10}}, true);

    // std::size_t
    EXPECT_EQ(std::size_t {element_id_t {10}}, std::size_t {10});
    EXPECT_THROW(static_cast<void>(std::size_t {null_element}), std::runtime_error);

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

TEST(VocabularyElementId, Hashing) {
    const auto hash = ankerl::unordered_dense::hash<element_id_t> {};

    EXPECT_TRUE(hash(element_id_t(1)) != hash(element_id_t(0)));
    EXPECT_TRUE(hash(element_id_t(1)) != hash(element_id_t(-1)));
    EXPECT_TRUE(hash(element_id_t(1)) == hash(element_id_t(1)));

    // avalanching
    EXPECT_TRUE(hash(element_id_t(1)) != 1);
}

}  // namespace logicsim
