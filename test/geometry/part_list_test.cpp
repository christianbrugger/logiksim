
#include "geometry/part_list.h"

#include "vocabulary.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>


namespace logicsim {

namespace {
auto part(offset_t::value_type a, offset_t::value_type b) -> part_t {
    return part_t {offset_t {a}, offset_t {b}};
}
}  // namespace
	

//
// Part Vectors
//

TEST(Geometry, AddPart) {
    {
        std::vector<part_t> entries {};
        add_part(entries, part(5, 10));
        ASSERT_THAT(entries, testing::ElementsAre(part(5, 10)));
    }
    {
        std::vector<part_t> entries {};
        add_part(entries, part(5, 10));
        add_part(entries, part(0, 2));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre(part(0, 2), part(5, 10)));
    }
    {
        std::vector<part_t> entries {};
        add_part(entries, part(5, 10));
        add_part(entries, part(0, 5));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre(part(0, 10)));
    }
    {
        std::vector<part_t> entries {};
        add_part(entries, part(5, 10));
        add_part(entries, part(0, 4));
        add_part(entries, part(4, 5));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre(part(0, 10)));
    }
    {
        std::vector<part_t> entries {};
        add_part(entries, part(5, 9));
        add_part(entries, part(1, 4));
        add_part(entries, part(0, 10));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre(part(0, 10)));
    }
    {
        std::vector<part_t> entries {};
        add_part(entries, part(5, 10));
        add_part(entries, part(3, 7));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre(part(3, 10)));
    }
}

TEST(Geometry, RemovePart) {
    {
        std::vector<part_t> entries {part(5, 10), part(20, 30)};
        remove_part(entries, part(5, 10));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre(part(20, 30)));
    }
    {
        std::vector<part_t> entries {part(5, 10), part(20, 30)};
        remove_part(entries, part(0, 2));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre(part(5, 10), part(20, 30)));
    }
    {
        std::vector<part_t> entries {part(5, 10), part(20, 30)};
        remove_part(entries, part(0, 100));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre());
    }
    {
        std::vector<part_t> entries {part(5, 10), part(20, 30)};
        remove_part(entries, part(10, 20));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre(part(5, 10), part(20, 30)));
    }
    {
        std::vector<part_t> entries {part(5, 10), part(20, 30)};
        remove_part(entries, part(8, 25));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre(part(5, 8), part(25, 30)));
    }
    {
        std::vector<part_t> entries {part(5, 10), part(20, 30)};
        remove_part(entries, part(6, 9));
        std::ranges::sort(entries);
        ASSERT_THAT(entries, testing::ElementsAre(part(5, 6), part(9, 10), part(20, 30)));
    }
}




}