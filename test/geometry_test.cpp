
#include "geometry.h"

#include "vocabulary.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>

namespace logicsim {

namespace {
auto part(offset_t::value_type a, offset_t::value_type b) -> part_t {
    return part_t {offset_t {a}, offset_t {b}};
}
}  // namespace

TEST(Geometry, AInsideB) {
    ASSERT_EQ(a_inside_b(part(1, 5), part(0, 10)), true);
    ASSERT_EQ(a_inside_b(part(1, 5), part(1, 5)), true);
    ASSERT_EQ(a_inside_b(part(1, 5), part(0, 5)), true);
    ASSERT_EQ(a_inside_b(part(1, 5), part(1, 6)), true);

    ASSERT_EQ(a_inside_b(part(1, 5), part(4, 10)), false);
    ASSERT_EQ(a_inside_b(part(1, 5), part(0, 2)), false);
    ASSERT_EQ(a_inside_b(part(1, 5), part(0, 1)), false);
    ASSERT_EQ(a_inside_b(part(1, 5), part(6, 10)), false);
}

TEST(Geometry, AInsideBNotTouching) {
    ASSERT_EQ(a_inside_b_not_touching(part(1, 5), part(0, 10)), true);

    ASSERT_EQ(a_inside_b_not_touching(part(1, 5), part(1, 5)), false);
    ASSERT_EQ(a_inside_b_not_touching(part(1, 5), part(0, 5)), false);
    ASSERT_EQ(a_inside_b_not_touching(part(1, 5), part(1, 6)), false);

    ASSERT_EQ(a_inside_b_not_touching(part(1, 5), part(4, 10)), false);
    ASSERT_EQ(a_inside_b_not_touching(part(1, 5), part(0, 2)), false);
    ASSERT_EQ(a_inside_b_not_touching(part(1, 5), part(0, 1)), false);
    ASSERT_EQ(a_inside_b_not_touching(part(1, 5), part(6, 10)), false);
}

TEST(Geometry, AInsideBTouchingOneSide) {
    ASSERT_EQ(a_inside_b_touching_one_side(part(1, 5), part(0, 10)), false);
    ASSERT_EQ(a_inside_b_touching_one_side(part(1, 5), part(1, 5)), false);

    ASSERT_EQ(a_inside_b_touching_one_side(part(1, 5), part(0, 5)), true);
    ASSERT_EQ(a_inside_b_touching_one_side(part(1, 5), part(1, 6)), true);

    ASSERT_EQ(a_inside_b_touching_one_side(part(1, 5), part(4, 10)), false);
    ASSERT_EQ(a_inside_b_touching_one_side(part(1, 5), part(0, 2)), false);
    ASSERT_EQ(a_inside_b_touching_one_side(part(1, 5), part(0, 1)), false);
    ASSERT_EQ(a_inside_b_touching_one_side(part(1, 5), part(6, 10)), false);
}

TEST(Geometry, AInsideBTouchingBegin) {
    ASSERT_EQ(a_inside_b_touching_begin(part(1, 5), part(0, 10)), false);
    ASSERT_EQ(a_inside_b_touching_begin(part(1, 5), part(1, 5)), false);

    ASSERT_EQ(a_inside_b_touching_begin(part(1, 5), part(0, 5)), false);
    ASSERT_EQ(a_inside_b_touching_begin(part(1, 5), part(1, 6)), true);

    ASSERT_EQ(a_inside_b_touching_begin(part(1, 5), part(4, 10)), false);
    ASSERT_EQ(a_inside_b_touching_begin(part(1, 5), part(0, 2)), false);
    ASSERT_EQ(a_inside_b_touching_begin(part(1, 5), part(0, 1)), false);
    ASSERT_EQ(a_inside_b_touching_begin(part(1, 5), part(6, 10)), false);
}

TEST(Geometry, AInsideBTouchingEnd) {
    ASSERT_EQ(a_inside_b_touching_end(part(1, 5), part(0, 10)), false);
    ASSERT_EQ(a_inside_b_touching_end(part(1, 5), part(1, 5)), false);

    ASSERT_EQ(a_inside_b_touching_end(part(1, 5), part(0, 5)), true);
    ASSERT_EQ(a_inside_b_touching_end(part(1, 5), part(1, 6)), false);

    ASSERT_EQ(a_inside_b_touching_end(part(1, 5), part(4, 10)), false);
    ASSERT_EQ(a_inside_b_touching_end(part(1, 5), part(0, 2)), false);
    ASSERT_EQ(a_inside_b_touching_end(part(1, 5), part(0, 1)), false);
    ASSERT_EQ(a_inside_b_touching_end(part(1, 5), part(6, 10)), false);
}

TEST(Geometry, ADisjointToB) {
    ASSERT_EQ(a_disjoint_b(part(1, 5), part(0, 10)), false);
    ASSERT_EQ(a_disjoint_b(part(1, 5), part(1, 5)), false);

    ASSERT_EQ(a_disjoint_b(part(1, 5), part(0, 5)), false);
    ASSERT_EQ(a_disjoint_b(part(1, 5), part(1, 6)), false);

    ASSERT_EQ(a_disjoint_b(part(1, 5), part(4, 10)), false);
    ASSERT_EQ(a_disjoint_b(part(1, 5), part(0, 2)), false);

    ASSERT_EQ(a_disjoint_b(part(1, 5), part(0, 1)), true);
    ASSERT_EQ(a_disjoint_b(part(1, 5), part(6, 10)), true);
}

TEST(Geometry, AEqualB) {
    ASSERT_EQ(a_equal_b(part(1, 5), part(0, 10)), false);
    ASSERT_EQ(a_equal_b(part(1, 5), part(1, 5)), true);

    ASSERT_EQ(a_equal_b(part(1, 5), part(0, 5)), false);
    ASSERT_EQ(a_equal_b(part(1, 5), part(1, 6)), false);

    ASSERT_EQ(a_equal_b(part(1, 5), part(4, 10)), false);
    ASSERT_EQ(a_equal_b(part(1, 5), part(0, 2)), false);

    ASSERT_EQ(a_equal_b(part(1, 5), part(0, 1)), false);
    ASSERT_EQ(a_equal_b(part(1, 5), part(6, 10)), false);
}

TEST(Geometry, AOverlappsAnyOfB) {
    ASSERT_EQ(a_overlapps_any_of_b(part(1, 5), part(0, 10)), true);
    ASSERT_EQ(a_overlapps_any_of_b(part(1, 5), part(1, 5)), true);

    ASSERT_EQ(a_overlapps_any_of_b(part(1, 5), part(0, 5)), true);
    ASSERT_EQ(a_overlapps_any_of_b(part(1, 5), part(1, 6)), true);

    ASSERT_EQ(a_overlapps_any_of_b(part(1, 5), part(4, 10)), true);
    ASSERT_EQ(a_overlapps_any_of_b(part(1, 5), part(0, 2)), true);

    ASSERT_EQ(a_overlapps_any_of_b(part(1, 5), part(0, 1)), false);
    ASSERT_EQ(a_overlapps_any_of_b(part(1, 5), part(6, 10)), false);
}

TEST(Geometry, AOverlappsBBegin) {
    ASSERT_EQ(a_overlapps_b_begin(part(1, 5), part(0, 10)), false);
    ASSERT_EQ(a_overlapps_b_begin(part(1, 5), part(1, 5)), false);

    ASSERT_EQ(a_overlapps_b_begin(part(1, 5), part(1, 6)), true);
    ASSERT_EQ(a_overlapps_b_begin(part(1, 5), part(4, 10)), true);

    ASSERT_EQ(a_overlapps_b_begin(part(1, 5), part(0, 5)), false);
    ASSERT_EQ(a_overlapps_b_begin(part(1, 5), part(0, 2)), false);

    ASSERT_EQ(a_overlapps_b_begin(part(1, 5), part(0, 1)), false);
    ASSERT_EQ(a_overlapps_b_begin(part(1, 5), part(6, 10)), false);
}

TEST(Geometry, AOverlappBEnd) {
    ASSERT_EQ(a_overlapps_b_end(part(1, 5), part(0, 10)), false);
    ASSERT_EQ(a_overlapps_b_end(part(1, 5), part(1, 5)), false);

    ASSERT_EQ(a_overlapps_b_end(part(1, 5), part(1, 6)), false);
    ASSERT_EQ(a_overlapps_b_end(part(1, 5), part(4, 10)), false);

    ASSERT_EQ(a_overlapps_b_end(part(1, 5), part(0, 2)), true);
    ASSERT_EQ(a_overlapps_b_end(part(1, 5), part(0, 5)), true);

    ASSERT_EQ(a_overlapps_b_end(part(1, 5), part(0, 1)), false);
    ASSERT_EQ(a_overlapps_b_end(part(1, 5), part(6, 10)), false);
}

//
// To Part
//

TEST(Geometry, ToPartLine) {
    ASSERT_EQ(to_part(ordered_line_t {point_t {1, 2}, point_t {3, 2}}), part(0, 2));
    ASSERT_EQ(to_part(ordered_line_t {point_t {0, 5}, point_t {100, 5}}), part(0, 100));
    ASSERT_EQ(to_part(ordered_line_t {point_t {-5, -1}, point_t {0, -1}}), part(0, 5));

    ASSERT_EQ(to_part(ordered_line_t {point_t {2, 1}, point_t {2, 3}}), part(0, 2));
    ASSERT_EQ(to_part(ordered_line_t {point_t {5, 0}, point_t {5, 100}}), part(0, 100));
    ASSERT_EQ(to_part(ordered_line_t {point_t {-1, -5}, point_t {-1, 0}}), part(0, 5));
}

TEST(Geometry, ToPartLineLine) {
    EXPECT_THROW(
        static_cast<void>(to_part(ordered_line_t {point_t {1, 2}, point_t {3, 2}},
                                  ordered_line_t {point_t {1, 2}, point_t {4, 2}})),
        std::runtime_error);
    EXPECT_THROW(
        static_cast<void>(to_part(ordered_line_t {point_t {1, 2}, point_t {3, 2}},
                                  ordered_line_t {point_t {0, 2}, point_t {3, 2}})),
        std::runtime_error);

    ASSERT_EQ(to_part(ordered_line_t {point_t {5, 1}, point_t {10, 1}},
                      ordered_line_t {point_t {5, 1}, point_t {6, 1}}),
              part(0, 1));

    ASSERT_EQ(to_part(ordered_line_t {point_t {5, 1}, point_t {10, 1}},
                      ordered_line_t {point_t {9, 1}, point_t {10, 1}}),
              part(4, 5));
}

TEST(Geometry, ToPartLineRect) {
    {
        const auto line = ordered_line_t {point_t {5, 1}, point_t {10, 1}};
        const auto rect = rect_fine_t {point_fine_t {0, 0}, point_fine_t {10, 10}};
        const auto res = std::optional {part(0, 5)};
        ASSERT_EQ(to_part(line, rect), res);
    }

    {
        const auto line = ordered_line_t {point_t {5, 1}, point_t {10, 1}};
        const auto rect = rect_fine_t {point_fine_t {6, 0}, point_fine_t {7, 10}};
        const auto res = std::optional {part(1, 2)};
        ASSERT_EQ(to_part(line, rect), res);
    }

    {
        const auto line = ordered_line_t {point_t {5, 1}, point_t {10, 1}};
        const auto rect = rect_fine_t {point_fine_t {0, 0}, point_fine_t {5, 10}};
        const auto res = std::nullopt;
        ASSERT_EQ(to_part(line, rect), res);
    }

    {
        const auto line = ordered_line_t {point_t {5, 1}, point_t {10, 1}};
        const auto rect = rect_fine_t {point_fine_t {5.5, 0}, point_fine_t {7.5, 10}};
        const auto res = std::optional {part(0, 3)};
        ASSERT_EQ(to_part(line, rect), res);
    }
}

TEST(Geometry, ToLineLinePart) {
    EXPECT_THROW(static_cast<void>(to_line(
                     ordered_line_t {point_t {1, 2}, point_t {3, 2}}, part(0, 10))),
                 std::runtime_error);

    {
        const auto res = ordered_line_t {point_t {5, 1}, point_t {6, 1}};
        ASSERT_EQ(to_line(ordered_line_t {point_t {5, 1}, point_t {10, 1}}, part(0, 1)),
                  res);
    }

    {
        const auto res = ordered_line_t {point_t {9, 1}, point_t {10, 1}};
        ASSERT_EQ(to_line(ordered_line_t {point_t {5, 1}, point_t {10, 1}}, part(4, 5)),
                  res);
    }
}

TEST(Geometry, IsPartValid) {
    ASSERT_EQ(is_part_valid(part(0, 5), ordered_line_t {point_t {5, 1}, point_t {10, 1}}),
              true);
    ASSERT_EQ(is_part_valid(part(0, 6), ordered_line_t {point_t {5, 1}, point_t {10, 1}}),
              false);
}

//
// intersect
//

TEST(Geometry, Intersect) {
    ASSERT_EQ(intersect(part(1, 5), part(0, 10)), std::optional {part(1, 5)});
    ASSERT_EQ(intersect(part(1, 5), part(1, 5)), std::optional {part(1, 5)});

    ASSERT_EQ(intersect(part(1, 5), part(0, 5)), std::optional {part(1, 5)});
    ASSERT_EQ(intersect(part(1, 5), part(1, 6)), std::optional {part(1, 5)});

    ASSERT_EQ(intersect(part(1, 5), part(4, 10)), std::optional {part(4, 5)});
    ASSERT_EQ(intersect(part(1, 5), part(0, 2)), std::optional {part(1, 2)});

    ASSERT_EQ(intersect(part(1, 5), part(0, 1)), std::nullopt);
    ASSERT_EQ(intersect(part(1, 5), part(6, 10)), std::nullopt);
}

TEST(Geometry, DifferenceTouchingOneSide) {
    EXPECT_THROW(static_cast<void>(difference_touching_one_side(part(0, 10), part(1, 5))),
                 std::runtime_error);
    EXPECT_THROW(static_cast<void>(difference_touching_one_side(part(1, 5), part(1, 5))),
                 std::runtime_error);

    ASSERT_EQ(difference_touching_one_side(part(0, 5), part(1, 5)), part(0, 1));
    ASSERT_EQ(difference_touching_one_side(part(1, 6), part(1, 5)), part(5, 6));

    EXPECT_THROW(static_cast<void>(difference_touching_one_side(part(4, 10), part(1, 5))),
                 std::runtime_error);
    EXPECT_THROW(static_cast<void>(difference_touching_one_side(part(0, 2), part(1, 5))),
                 std::runtime_error);

    EXPECT_THROW(static_cast<void>(difference_touching_one_side(part(0, 1), part(1, 5))),
                 std::runtime_error);
    EXPECT_THROW(static_cast<void>(difference_touching_one_side(part(6, 10), part(1, 5))),
                 std::runtime_error);
}

TEST(Geometry, DifferenceNotTouching) {
    ASSERT_EQ(difference_not_touching(part(0, 10), part(1, 5)),
              std::make_pair(part(0, 1), part(5, 10)));
    EXPECT_THROW(static_cast<void>(difference_not_touching(part(1, 5), part(1, 5))),
                 std::runtime_error);

    EXPECT_THROW(static_cast<void>(difference_not_touching(part(0, 5), part(1, 5))),
                 std::runtime_error);
    EXPECT_THROW(static_cast<void>(difference_not_touching(part(1, 6), part(1, 5))),
                 std::runtime_error);

    EXPECT_THROW(static_cast<void>(difference_not_touching(part(4, 10), part(1, 5))),
                 std::runtime_error);
    EXPECT_THROW(static_cast<void>(difference_not_touching(part(0, 2), part(1, 5))),
                 std::runtime_error);

    EXPECT_THROW(static_cast<void>(difference_not_touching(part(0, 1), part(1, 5))),
                 std::runtime_error);
    EXPECT_THROW(static_cast<void>(difference_not_touching(part(6, 10), part(1, 5))),
                 std::runtime_error);
}

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
