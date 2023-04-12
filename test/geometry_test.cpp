
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

TEST(Geometry, ADisjointToB) {
    ASSERT_EQ(a_disjoint_to_b(part(1, 5), part(0, 10)), false);
    ASSERT_EQ(a_disjoint_to_b(part(1, 5), part(1, 5)), false);

    ASSERT_EQ(a_disjoint_to_b(part(1, 5), part(0, 5)), false);
    ASSERT_EQ(a_disjoint_to_b(part(1, 5), part(1, 6)), false);

    ASSERT_EQ(a_disjoint_to_b(part(1, 5), part(4, 10)), false);
    ASSERT_EQ(a_disjoint_to_b(part(1, 5), part(0, 2)), false);

    ASSERT_EQ(a_disjoint_to_b(part(1, 5), part(0, 1)), true);
    ASSERT_EQ(a_disjoint_to_b(part(1, 5), part(6, 10)), true);
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

TEST(Geometry, AOverlappsB) {
    ASSERT_EQ(a_overlapps_b(part(1, 5), part(0, 10)), true);
    ASSERT_EQ(a_overlapps_b(part(1, 5), part(1, 5)), true);

    ASSERT_EQ(a_overlapps_b(part(1, 5), part(0, 5)), true);
    ASSERT_EQ(a_overlapps_b(part(1, 5), part(1, 6)), true);

    ASSERT_EQ(a_overlapps_b(part(1, 5), part(4, 10)), true);
    ASSERT_EQ(a_overlapps_b(part(1, 5), part(0, 2)), true);

    ASSERT_EQ(a_overlapps_b(part(1, 5), part(0, 1)), false);
    ASSERT_EQ(a_overlapps_b(part(1, 5), part(6, 10)), false);
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

}  // namespace logicsim