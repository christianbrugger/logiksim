
#include "part_selection.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

TEST(PartSelection, SimpleMembers) {
    {
        auto entries = PartSelection {};
        EXPECT_EQ(entries.empty(), true);
        EXPECT_EQ(entries.size(), 0);
        EXPECT_EQ(entries.begin(), entries.end());
    }
    {
        auto entries = PartSelection {part_t(10, 20)};
        EXPECT_EQ(entries.empty(), false);
        EXPECT_EQ(entries.size(), 1);
        EXPECT_NE(entries.begin(), entries.end());
        EXPECT_THAT(entries, testing::ElementsAre(part_t(10, 20)));
    }
    {
        auto entries = PartSelection {{part_t(10, 20), part_t(0, 5)}};
        EXPECT_EQ(entries.empty(), false);
        EXPECT_EQ(entries.size(), 2);
        EXPECT_THAT(entries, testing::ElementsAre(part_t(0, 5), part_t(10, 20)));
    }
    {
        auto entries = PartSelection {{part_t(10, 20), part_t(5, 10)}};
        EXPECT_EQ(entries.empty(), false);
        EXPECT_EQ(entries.size(), 1);
        EXPECT_THAT(entries, testing::ElementsAre(part_t(5, 20)));
    }
}

TEST(PartSelection, MaxOffset) {
    {
        auto entries = PartSelection {};
        EXPECT_EQ(entries.max_offset(), offset_t {0});
    }
    {
        auto entries = PartSelection {part_t(5, 10)};
        EXPECT_EQ(entries.max_offset(), offset_t {10});
    }
    {
        auto entries = PartSelection {{part_t(5, 10), part_t(15, 20)}};
        EXPECT_EQ(entries.max_offset(), offset_t {20});
    }
}

TEST(PartSelection, AddPart) {
    {
        auto entries = PartSelection {};
        entries.add_part(part_t(5, 10));
        EXPECT_THAT(entries, testing::ElementsAre(part_t(5, 10)));
    }
    {
        auto entries = PartSelection {};
        entries.add_part(part_t(5, 10));
        entries.add_part(part_t(0, 2));
        EXPECT_THAT(entries, testing::ElementsAre(part_t(0, 2), part_t(5, 10)));
    }
    {
        auto entries = PartSelection {};
        entries.add_part(part_t(5, 10));
        entries.add_part(part_t(0, 5));
        EXPECT_THAT(entries, testing::ElementsAre(part_t(0, 10)));
    }
    {
        auto entries = PartSelection {};
        entries.add_part(part_t(5, 10));
        entries.add_part(part_t(0, 4));
        entries.add_part(part_t(4, 5));
        EXPECT_THAT(entries, testing::ElementsAre(part_t(0, 10)));
    }
    {
        auto entries = PartSelection {};
        entries.add_part(part_t(5, 9));
        entries.add_part(part_t(1, 4));
        entries.add_part(part_t(0, 10));
        EXPECT_THAT(entries, testing::ElementsAre(part_t(0, 10)));
    }
    {
        auto entries = PartSelection {};
        entries.add_part(part_t(5, 10));
        entries.add_part(part_t(3, 7));
        EXPECT_THAT(entries, testing::ElementsAre(part_t(3, 10)));
    }
}

TEST(PartSelection, RemovePart) {
    {
        auto entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        entries.remove_part(part_t(5, 10));
        EXPECT_THAT(entries, testing::ElementsAre(part_t(20, 30)));
    }
    {
        auto entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        entries.remove_part(part_t(0, 2));
        EXPECT_THAT(entries, testing::ElementsAre(part_t(5, 10), part_t(20, 30)));
    }
    {
        auto entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        entries.remove_part(part_t(0, 100));
        EXPECT_THAT(entries, testing::ElementsAre());
    }
    {
        auto entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        entries.remove_part(part_t(10, 20));
        EXPECT_THAT(entries, testing::ElementsAre(part_t(5, 10), part_t(20, 30)));
    }
    {
        auto entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        entries.remove_part(part_t(8, 25));
        EXPECT_THAT(entries, testing::ElementsAre(part_t(5, 8), part_t(25, 30)));
    }
    {
        auto entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        entries.remove_part(part_t(6, 9));
        EXPECT_THAT(entries,
                    testing::ElementsAre(part_t(5, 6), part_t(9, 10), part_t(20, 30)));
    }
}

TEST(PartSelection, CopyPartMember) {
    {
        const auto source_entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        auto destination = PartSelection({part_t(7, 15)});

        destination.copy_parts(source_entries, part_copy_definition_t {
                                                   .destination = part_t(0, 10),
                                                   .source = part_t(0, 10),
                                               });
        EXPECT_THAT(destination, testing::ElementsAre(part_t(5, 15)));
    }
    {
        const auto source_entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        auto destination = PartSelection({part_t(7, 10)});
        destination.copy_parts(source_entries, part_copy_definition_t {
                                                   .destination = part_t(5, 20),
                                                   .source = part_t(0, 15),
                                               });
        EXPECT_THAT(destination, testing::ElementsAre(part_t(7, 15)));
    }
    {
        const auto source_entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        auto destination = PartSelection({part_t(10, 20)});

        destination.copy_parts(source_entries, part_copy_definition_t {
                                                   .destination = part_t(0, 40),
                                                   .source = part_t(0, 40),
                                               });
        EXPECT_THAT(destination, testing::ElementsAre(part_t(5, 30)));
    }
    {
        const auto source_entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        auto destination = PartSelection({part_t(5, 10)});

        destination.copy_parts(source_entries, part_copy_definition_t {
                                                   .destination = part_t(0, 5),
                                                   .source = part_t(5, 10),
                                               });
        EXPECT_THAT(destination, testing::ElementsAre(part_t(0, 10)));
    }
}

TEST(PartSelection, CopyPartFreeFunction_1) {
    {
        const auto source_entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        const auto destination =
            copy_parts(source_entries, part_copy_definition_t {
                                           .destination = part_t(0, 10),
                                           .source = part_t(0, 10),
                                       });
        EXPECT_THAT(destination, testing::ElementsAre(part_t(5, 10)));
    }
    {
        const auto source_entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        const auto destination =
            copy_parts(source_entries, part_copy_definition_t {
                                           .destination = part_t(5, 15),
                                           .source = part_t(0, 10),
                                       });
        EXPECT_THAT(destination, testing::ElementsAre(part_t(10, 15)));
    }
    {
        const auto source_entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        const auto destination =
            copy_parts(source_entries, part_copy_definition_t {
                                           .destination = part_t(5, 10),
                                           .source = part_t(0, 5),
                                       });
        EXPECT_THAT(destination, testing::ElementsAre());
    }
    {
        const auto source_entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        const auto destination =
            copy_parts(source_entries, part_copy_definition_t {
                                           .destination = part_t(5, 30),
                                           .source = part_t(0, 25),
                                       });
        EXPECT_THAT(destination, testing::ElementsAre(part_t(10, 15), part_t(25, 30)));
    }
}

TEST(PartSelection, CopyPartFreeFunction_2) {
    {
        const auto source_entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        const auto copy_definition = part_copy_definition_t {
            .destination = part_t(0, 10),
            .source = part_t(5, 10),
        };

        EXPECT_THROW(static_cast<void>(copy_parts(source_entries, copy_definition)),
                     std::runtime_error);
    }
    {
        const auto source_entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        const auto copy_definition = part_copy_definition_t {
            .destination = part_t(5, 10),
            .source = part_t(0, 10),
        };

        EXPECT_THROW(static_cast<void>(copy_parts(source_entries, copy_definition)),
                     std::runtime_error);
    }

    {
        const auto source_entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        const auto copy_definition = part_copy_definition_t {
            .destination = part_t(0, 5),
            .source = part_t(5, 10),
        };
        const auto destination = copy_parts(source_entries, copy_definition);

        EXPECT_THAT(destination, testing::ElementsAre(part_t(0, 5)));
    }
    {
        const auto source_entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        const auto copy_definition = part_copy_definition_t {
            .destination = part_t(5, 10),
            .source = part_t(5, 10),
        };
        const auto destination = copy_parts(source_entries, copy_definition);

        EXPECT_THAT(destination, testing::ElementsAre(part_t(5, 10)));
    }
    {
        const auto source_entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        const auto copy_definition = part_copy_definition_t {
            .destination = part_t(6, 20),
            .source = part_t(8, 22),
        };
        const auto destination = copy_parts(source_entries, copy_definition);

        EXPECT_THAT(destination, testing::ElementsAre(part_t(6, 8), part_t(18, 20)));
    }
    {
        const auto source_entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        const auto copy_definition = part_copy_definition_t {
            .destination = part_t(0, 30),
            .source = part_t(0, 30),
        };
        const auto destination = copy_parts(source_entries, copy_definition);

        EXPECT_THAT(destination, testing::ElementsAre(part_t(5, 10), part_t(20, 30)));
    }
}

TEST(PartSelection, MoveParts) {
    {
        auto source_entries = PartSelection({part_t(5, 10), part_t(20, 30)});
        const auto copy_definition = part_copy_definition_t {
            .destination = part_t(0, 5),
            .source = part_t(5, 10),
        };
        auto destination = PartSelection({part_t(3, 10)});
        move_parts(move_definition_t {
            .destination = destination,
            .source = source_entries,
            .copy_definition = copy_definition,
        });

        EXPECT_THAT(source_entries, testing::ElementsAre(part_t(20, 30)));
        EXPECT_THAT(destination, testing::ElementsAre(part_t(0, 10)));
    }
    {
        auto source_entries = PartSelection({part_t(0, 15), part_t(20, 30)});
        const auto copy_definition = part_copy_definition_t {
            .destination = part_t(10, 15),
            .source = part_t(5, 10),
        };
        auto destination = PartSelection({part_t(0, 5)});
        move_parts(move_definition_t {
            .destination = destination,
            .source = source_entries,
            .copy_definition = copy_definition,
        });

        EXPECT_THAT(source_entries,
                    testing::ElementsAre(part_t(0, 5), part_t(10, 15), part_t(20, 30)));
        EXPECT_THAT(destination, testing::ElementsAre(part_t(0, 5), part_t(10, 15)));
    }
}

TEST(PartSelection, Invert) {
    {
        const auto source = PartSelection {};
        const auto inverted = PartSelection::inverted(source, part_t(0, 10));

        EXPECT_THAT(inverted, testing::ElementsAre(part_t(0, 10)));
    }
    {
        const auto source = PartSelection({part_t(5, 10), part_t(20, 30)});
        const auto inverted = PartSelection::inverted(source, part_t(0, 10));

        EXPECT_THAT(inverted, testing::ElementsAre(part_t(0, 5)));
    }
    {
        const auto source = PartSelection({part_t(5, 10), part_t(20, 30)});
        const auto inverted = PartSelection::inverted(source, part_t(0, 40));

        EXPECT_THAT(inverted,
                    testing::ElementsAre(part_t(0, 5), part_t(10, 20), part_t(30, 40)));
    }
    {
        const auto source = PartSelection({part_t(5, 10), part_t(20, 30)});
        const auto inverted = PartSelection::inverted(source, part_t(40, 50));

        EXPECT_THAT(inverted, testing::ElementsAre(part_t(40, 50)));
    }
    {
        const auto source = PartSelection({part_t(0, 10), part_t(20, 30)});
        const auto inverted = PartSelection::inverted(source, part_t(0, 10));

        EXPECT_THAT(inverted, testing::ElementsAre());
    }
    {
        const auto source = PartSelection({part_t(0, 10), part_t(20, 30)});
        const auto inverted = PartSelection::inverted(source, part_t(0, 25));

        EXPECT_THAT(inverted, testing::ElementsAre(part_t(10, 20)));
    }
    {
        const auto source = PartSelection({part_t(20, 30)});
        const auto inverted = PartSelection::inverted(source, part_t(0, 10));

        EXPECT_THAT(inverted, testing::ElementsAre(part_t(0, 10)));
    }
    {
        const auto source = PartSelection({part_t(0, 5), part_t(10, 15), part_t(20, 25),
                                           part_t(30, 35), part_t(40, 45)});
        const auto inverted = PartSelection::inverted(source, part_t(0, 45));

        EXPECT_THAT(inverted, testing::ElementsAre(part_t(5, 10), part_t(15, 20),
                                                   part_t(25, 30), part_t(35, 40)));
    }
}

}  // namespace logicsim
