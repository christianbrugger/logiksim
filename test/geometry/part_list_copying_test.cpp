
#include "geometry/part_list_copying.h"

#include "vocabulary.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

//
// part copying
//

TEST(Geometry, CopyPartReturn) {
    {
        const std::vector<part_t> source_entries {part_t(5, 10), part_t(20, 30)};
        auto destination = copy_parts(source_entries, part_t(0, 10));
        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part_t(5, 10)));
    }
    {
        const std::vector<part_t> source_entries {part_t(5, 10), part_t(20, 30)};
        auto destination = copy_parts(source_entries, part_t(5, 15));
        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part_t(10, 15)));
    }
    {
        const std::vector<part_t> source_entries {part_t(5, 10), part_t(20, 30)};
        auto destination = copy_parts(source_entries, part_t(5, 10));
        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre());
    }
    {
        const std::vector<part_t> source_entries {part_t(5, 10), part_t(20, 30)};
        auto destination = copy_parts(source_entries, part_t(5, 30));
        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part_t(10, 15), part_t(25, 30)));
    }
}

TEST(Geometry, CopyPartToDestination) {
    {
        const std::vector<part_t> source_entries {part_t(5, 10), part_t(20, 30)};
        auto destination = std::vector<part_t> {part_t(7, 15)};
        copy_parts(source_entries, destination, part_t(0, 10));

        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part_t(5, 15)));
    }
    {
        const std::vector<part_t> source_entries {part_t(5, 10), part_t(20, 30)};
        auto destination = std::vector<part_t> {part_t(7, 10)};
        copy_parts(source_entries, destination, part_t(5, 20));

        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part_t(7, 15)));
    }
    {
        const std::vector<part_t> source_entries {part_t(5, 10), part_t(20, 30)};
        auto destination = std::vector<part_t> {part_t(10, 20)};
        copy_parts(source_entries, destination, part_t(0, 40));

        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part_t(5, 30)));
    }
}

TEST(Geometry, CopyPartResultWithDefinition) {
    {
        const std::vector<part_t> source_entries {part_t(5, 10), part_t(20, 30)};
        const auto parts = part_copy_definition_t {
            .destination = part_t(0, 10),
            .source = part_t(5, 10),
        };

        EXPECT_THROW(static_cast<void>(copy_parts(source_entries, parts)),
                     std::runtime_error);
    }
    {
        const std::vector<part_t> source_entries {part_t(5, 10), part_t(20, 30)};
        const auto parts = part_copy_definition_t {
            .destination = part_t(5, 10),
            .source = part_t(0, 10),
        };

        EXPECT_THROW(static_cast<void>(copy_parts(source_entries, parts)),
                     std::runtime_error);
    }

    {
        const std::vector<part_t> source_entries {part_t(5, 10), part_t(20, 30)};
        const auto parts = part_copy_definition_t {
            .destination = part_t(0, 5),
            .source = part_t(5, 10),
        };
        auto destination = copy_parts(source_entries, parts);

        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part_t(0, 5)));
    }
    {
        const std::vector<part_t> source_entries {part_t(5, 10), part_t(20, 30)};
        const auto parts = part_copy_definition_t {
            .destination = part_t(5, 10),
            .source = part_t(5, 10),
        };
        auto destination = copy_parts(source_entries, parts);

        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part_t(5, 10)));
    }
    {
        const std::vector<part_t> source_entries {part_t(5, 10), part_t(20, 30)};
        const auto parts = part_copy_definition_t {
            .destination = part_t(6, 20),
            .source = part_t(8, 22),
        };
        auto destination = copy_parts(source_entries, parts);

        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part_t(6, 8), part_t(18, 20)));
    }
    {
        const std::vector<part_t> source_entries {part_t(5, 10), part_t(20, 30)};
        const auto parts = part_copy_definition_t {
            .destination = part_t(0, 30),
            .source = part_t(0, 30),
        };
        auto destination = copy_parts(source_entries, parts);

        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part_t(5, 10), part_t(20, 30)));
    }
}

TEST(Geometry, CopyPartToDestinationWithDefinition) {
    {
        const std::vector<part_t> source_entries {part_t(5, 10), part_t(20, 30)};
        const auto parts = part_copy_definition_t {
            .destination = part_t(0, 5),
            .source = part_t(5, 10),
        };
        auto destination = std::vector<part_t> {part_t(5, 10)};
        copy_parts(source_entries, destination, parts);

        std::ranges::sort(destination);
        ASSERT_THAT(destination, testing::ElementsAre(part_t(0, 10)));
    }
}

TEST(Geometry, MovePartsWithDefinition) {
    {
        std::vector<part_t> source_entries {part_t(5, 10), part_t(20, 30)};
        const auto parts = part_copy_definition_t {
            .destination = part_t(0, 5),
            .source = part_t(5, 10),
        };
        auto destination = std::vector<part_t> {part_t(3, 10)};
        move_parts(source_entries, destination, parts);

        std::ranges::sort(source_entries);
        std::ranges::sort(destination);
        ASSERT_THAT(source_entries, testing::ElementsAre(part_t(20, 30)));
        ASSERT_THAT(destination, testing::ElementsAre(part_t(0, 10)));
    }
    {
        std::vector<part_t> source_entries {part_t(0, 15), part_t(20, 30)};
        const auto parts = part_copy_definition_t {
            .destination = part_t(10, 15),
            .source = part_t(5, 10),
        };
        auto destination = std::vector<part_t> {part_t(0, 5)};
        move_parts(source_entries, destination, parts);

        std::ranges::sort(source_entries);
        std::ranges::sort(destination);
        ASSERT_THAT(source_entries,
                    testing::ElementsAre(part_t(0, 5), part_t(10, 15), part_t(20, 30)));
        ASSERT_THAT(destination, testing::ElementsAre(part_t(0, 5), part_t(10, 15)));
    }
}

}  // namespace logicsim