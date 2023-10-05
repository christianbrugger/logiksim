
#include "geometry/part_list_copying.h"

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
// part copying
//

TEST(Geometry, CopyPartReturn) {
    {
        const std::vector<part_t> source_entries {part(5, 10), part(20, 30)};
        auto destination = copy_parts(source_entries, part(0, 10));
        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part(5, 10)));
    }
    {
        const std::vector<part_t> source_entries {part(5, 10), part(20, 30)};
        auto destination = copy_parts(source_entries, part(5, 15));
        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part(10, 15)));
    }
    {
        const std::vector<part_t> source_entries {part(5, 10), part(20, 30)};
        auto destination = copy_parts(source_entries, part(5, 10));
        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre());
    }
    {
        const std::vector<part_t> source_entries {part(5, 10), part(20, 30)};
        auto destination = copy_parts(source_entries, part(5, 30));
        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part(10, 15), part(25, 30)));
    }
}

TEST(Geometry, CopyPartToDestination) {
    {
        const std::vector<part_t> source_entries {part(5, 10), part(20, 30)};
        auto destination = std::vector<part_t> {part(7, 15)};
        copy_parts(source_entries, destination, part(0, 10));

        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part(5, 15)));
    }
    {
        const std::vector<part_t> source_entries {part(5, 10), part(20, 30)};
        auto destination = std::vector<part_t> {part(7, 10)};
        copy_parts(source_entries, destination, part(5, 20));

        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part(7, 15)));
    }
    {
        const std::vector<part_t> source_entries {part(5, 10), part(20, 30)};
        auto destination = std::vector<part_t> {part(10, 20)};
        copy_parts(source_entries, destination, part(0, 40));

        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part(5, 30)));
    }
}

TEST(Geometry, CopyPartResultWithDefinition) {
    {
        const std::vector<part_t> source_entries {part(5, 10), part(20, 30)};
        const auto parts = part_copy_definition_t {
            .destination = part(0, 10),
            .source = part(5, 10),
        };

        EXPECT_THROW(static_cast<void>(copy_parts(source_entries, parts)),
                     std::runtime_error);
    }
    {
        const std::vector<part_t> source_entries {part(5, 10), part(20, 30)};
        const auto parts = part_copy_definition_t {
            .destination = part(5, 10),
            .source = part(0, 10),
        };

        EXPECT_THROW(static_cast<void>(copy_parts(source_entries, parts)),
                     std::runtime_error);
    }

    {
        const std::vector<part_t> source_entries {part(5, 10), part(20, 30)};
        const auto parts = part_copy_definition_t {
            .destination = part(0, 5),
            .source = part(5, 10),
        };
        auto destination = copy_parts(source_entries, parts);

        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part(0, 5)));
    }
    {
        const std::vector<part_t> source_entries {part(5, 10), part(20, 30)};
        const auto parts = part_copy_definition_t {
            .destination = part(5, 10),
            .source = part(5, 10),
        };
        auto destination = copy_parts(source_entries, parts);

        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part(5, 10)));
    }
    {
        const std::vector<part_t> source_entries {part(5, 10), part(20, 30)};
        const auto parts = part_copy_definition_t {
            .destination = part(6, 20),
            .source = part(8, 22),
        };
        auto destination = copy_parts(source_entries, parts);

        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part(6, 8), part(18, 20)));
    }
    {
        const std::vector<part_t> source_entries {part(5, 10), part(20, 30)};
        const auto parts = part_copy_definition_t {
            .destination = part(0, 30),
            .source = part(0, 30),
        };
        auto destination = copy_parts(source_entries, parts);

        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part(5, 10), part(20, 30)));
    }
}

TEST(Geometry, CopyPartToDestinationWithDefinition) {
    {
        const std::vector<part_t> source_entries {part(5, 10), part(20, 30)};
        const auto parts = part_copy_definition_t {
            .destination = part(0, 5),
            .source = part(5, 10),
        };
        auto destination = std::vector<part_t> {part(5, 10)};
        copy_parts(source_entries, destination, parts);

        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part(0, 10)));
    }
}

TEST(Geometry, MovePartsWithDefinition) {
    {
        std::vector<part_t> source_entries {part(5, 10), part(20, 30)};
        const auto parts = part_copy_definition_t {
            .destination = part(0, 5),
            .source = part(5, 10),
        };
        auto destination = std::vector<part_t> {part(3, 10)};
        move_parts(source_entries, destination, parts);

        std::ranges::sort(source_entries);
        std::ranges::sort(destination);
        ASSERT_THAT(source_entries, testing::ElementsAre(part(20, 30)));
        ASSERT_THAT(destination, testing::ElementsAre(part(0, 10)));
    }
    {
        std::vector<part_t> source_entries {part(0, 15), part(20, 30)};
        const auto parts = part_copy_definition_t {
            .destination = part(10, 15),
            .source = part(5, 10),
        };
        auto destination = std::vector<part_t> {part(0, 5)};
        move_parts(source_entries, destination, parts);

        std::ranges::sort(source_entries);
        std::ranges::sort(destination);
        ASSERT_THAT(source_entries,
                    testing::ElementsAre(part(0, 5), part(10, 15), part(20, 30)));
        ASSERT_THAT(destination, testing::ElementsAre(part(0, 5), part(10, 15)));
    }
}

}  // namespace logicsim