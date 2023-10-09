
#include "geometry/part_list.h"

#include "vocabulary.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

//
// Part Vectors
//

TEST(Geometry, AddPart) {
    {
        std::vector<part_t> entries {};
        add_part(entries, part_t(5, 10));
        ASSERT_THAT(entries, testing::ElementsAre(part_t(5, 10)));
    }
    {
        std::vector<part_t> entries {};
        add_part(entries, part_t(5, 10));
        add_part(entries, part_t(0, 2));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre(part_t(0, 2), part_t(5, 10)));
    }
    {
        std::vector<part_t> entries {};
        add_part(entries, part_t(5, 10));
        add_part(entries, part_t(0, 5));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre(part_t(0, 10)));
    }
    {
        std::vector<part_t> entries {};
        add_part(entries, part_t(5, 10));
        add_part(entries, part_t(0, 4));
        add_part(entries, part_t(4, 5));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre(part_t(0, 10)));
    }
    {
        std::vector<part_t> entries {};
        add_part(entries, part_t(5, 9));
        add_part(entries, part_t(1, 4));
        add_part(entries, part_t(0, 10));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre(part_t(0, 10)));
    }
    {
        std::vector<part_t> entries {};
        add_part(entries, part_t(5, 10));
        add_part(entries, part_t(3, 7));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre(part_t(3, 10)));
    }
}

TEST(Geometry, RemovePart) {
    {
        std::vector<part_t> entries {part_t(5, 10), part_t(20, 30)};
        remove_part(entries, part_t(5, 10));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre(part_t(20, 30)));
    }
    {
        std::vector<part_t> entries {part_t(5, 10), part_t(20, 30)};
        remove_part(entries, part_t(0, 2));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre(part_t(5, 10), part_t(20, 30)));
    }
    {
        std::vector<part_t> entries {part_t(5, 10), part_t(20, 30)};
        remove_part(entries, part_t(0, 100));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre());
    }
    {
        std::vector<part_t> entries {part_t(5, 10), part_t(20, 30)};
        remove_part(entries, part_t(10, 20));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre(part_t(5, 10), part_t(20, 30)));
    }
    {
        std::vector<part_t> entries {part_t(5, 10), part_t(20, 30)};
        remove_part(entries, part_t(8, 25));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre(part_t(5, 8), part_t(25, 30)));
    }
    {
        std::vector<part_t> entries {part_t(5, 10), part_t(20, 30)};
        remove_part(entries, part_t(6, 9));
        std::ranges::sort(entries);
        ASSERT_THAT(entries,
                    testing::ElementsAre(part_t(5, 6), part_t(9, 10), part_t(20, 30)));
    }
}

}  // namespace logicsim