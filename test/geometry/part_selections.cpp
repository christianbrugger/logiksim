
#include "geometry/part_selections.h"

#include "format/container.h"
#include "format/std_type.h"
#include "logging.h"
#include "part_selection.h"
#include "vocabulary/part.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

namespace {

auto to_selection(std::initializer_list<part_t> list) -> PartSelection {
    return PartSelection {part_selection::part_vector_t {list}};
}

}  // namespace

TEST(GeometryPartSelections, PartOverlapsAnyOfSelection0) {
    ASSERT_EQ(a_overlaps_any_of_b(part_t {15, 20}, to_selection({})), false);
}

TEST(GeometryPartSelections, PartOverlapsAnyOfSelection1) {
    ASSERT_EQ(a_overlaps_any_of_b(part_t {3, 6}, to_selection({part_t {5, 10}})), true);
    ASSERT_EQ(a_overlaps_any_of_b(part_t {5, 10}, to_selection({part_t {5, 10}})), true);
    ASSERT_EQ(a_overlaps_any_of_b(part_t {6, 10}, to_selection({part_t {5, 10}})), true);
    ASSERT_EQ(a_overlaps_any_of_b(part_t {5, 9}, to_selection({part_t {5, 10}})), true);
    ASSERT_EQ(a_overlaps_any_of_b(part_t {6, 9}, to_selection({part_t {5, 10}})), true);
    ASSERT_EQ(a_overlaps_any_of_b(part_t {9, 15}, to_selection({part_t {5, 10}})), true);

    ASSERT_EQ(a_overlaps_any_of_b(part_t {0, 5}, to_selection({part_t {5, 10}})), false);
    ASSERT_EQ(a_overlaps_any_of_b(part_t {0, 4}, to_selection({part_t {5, 10}})), false);
    ASSERT_EQ(a_overlaps_any_of_b(part_t {10, 15}, to_selection({part_t {5, 10}})),
              false);
    ASSERT_EQ(a_overlaps_any_of_b(part_t {15, 20}, to_selection({part_t {5, 10}})),
              false);
}

TEST(GeometryPartSelections, PartOverlapsAnyOfSelection2) {
    ASSERT_EQ(a_overlaps_any_of_b(
                  part_t {0, 6},
                  to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
              true);
    ASSERT_EQ(a_overlaps_any_of_b(
                  part_t {16, 19},
                  to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
              true);
    ASSERT_EQ(a_overlaps_any_of_b(
                  part_t {29, 30},
                  to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
              true);
    ASSERT_EQ(a_overlaps_any_of_b(
                  part_t {5, 30},
                  to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
              true);
    ASSERT_EQ(a_overlaps_any_of_b(
                  part_t {15, 20},
                  to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
              true);

    ASSERT_EQ(a_overlaps_any_of_b(
                  part_t {0, 5},
                  to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
              false);
    ASSERT_EQ(a_overlaps_any_of_b(
                  part_t {10, 15},
                  to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
              false);
    ASSERT_EQ(a_overlaps_any_of_b(
                  part_t {20, 25},
                  to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
              false);
    ASSERT_EQ(a_overlaps_any_of_b(
                  part_t {35, 40},
                  to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
              false);
}

//
// overlaps - PartSelection & PartSelection
//

TEST(GeometryPartSelections, SelectionOverlapsAnyOfSelection0) {
    ASSERT_EQ(a_overlaps_any_of_b(to_selection({}), to_selection({part_t {5, 10}})),
              false);
    ASSERT_EQ(a_overlaps_any_of_b(to_selection({part_t {10, 20}}), to_selection({})),
              false);
    ASSERT_EQ(a_overlaps_any_of_b(to_selection({}), to_selection({})), false);
}

TEST(GeometryPartSelections, SelectionOverlapsAnyOfSelection1) {
    ASSERT_EQ(a_overlaps_any_of_b(to_selection({part_t {3, 6}}),
                                  to_selection({part_t {5, 10}})),
              true);
    ASSERT_EQ(a_overlaps_any_of_b(to_selection({part_t {6, 9}}),
                                  to_selection({part_t {5, 10}})),
              true);
    ASSERT_EQ(a_overlaps_any_of_b(to_selection({part_t {5, 10}}),
                                  to_selection({part_t {5, 10}})),
              true);

    ASSERT_EQ(a_overlaps_any_of_b(to_selection({part_t {0, 5}}),
                                  to_selection({part_t {5, 10}})),
              false);
    ASSERT_EQ(a_overlaps_any_of_b(to_selection({part_t {10, 20}}),
                                  to_selection({part_t {5, 10}})),
              false);
}

TEST(GeometryPartSelections, SelectionOverlapsAnyOfSelection2) {
    ASSERT_EQ(a_overlaps_any_of_b(
                  to_selection({part_t {3, 6}}),
                  to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
              true);
    ASSERT_EQ(a_overlaps_any_of_b(
                  to_selection({part_t {15, 16}}),
                  to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
              true);
    ASSERT_EQ(a_overlaps_any_of_b(
                  to_selection({part_t {29, 30}}),
                  to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
              true);

    ASSERT_EQ(a_overlaps_any_of_b(
                  to_selection({part_t {0, 3}}),
                  to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
              false);
    ASSERT_EQ(a_overlaps_any_of_b(
                  to_selection({part_t {20, 25}}),
                  to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
              false);
    ASSERT_EQ(a_overlaps_any_of_b(
                  to_selection({part_t {35, 40}}),
                  to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
              false);
}

TEST(GeometryPartSelections, SelectionOverlapsAnyOfSelection3) {
    ASSERT_EQ(
        a_overlaps_any_of_b(
            to_selection({part_t {0, 1}, part_t {2, 3}, part_t {4, 5}, part_t {15, 16}}),
            to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
        true);
    ASSERT_EQ(
        a_overlaps_any_of_b(
            to_selection({part_t {0, 1}, part_t {2, 3}, part_t {4, 5}, part_t {6, 7}}),
            to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
        true);
    ASSERT_EQ(
        a_overlaps_any_of_b(
            to_selection({part_t {0, 1}, part_t {2, 3}, part_t {4, 5}, part_t {29, 30}}),
            to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
        true);
    ASSERT_EQ(a_overlaps_any_of_b(
                  to_selection({part_t {24, 26}, part_t {40, 41}, part_t {43, 44},
                                part_t {50, 51}}),
                  to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
              true);

    ASSERT_EQ(a_overlaps_any_of_b(
                  to_selection({part_t {0, 1}, part_t {2, 3}, part_t {4, 5}}),
                  to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
              false);
    ASSERT_EQ(a_overlaps_any_of_b(
                  to_selection({part_t {0, 1}, part_t {2, 3}, part_t {4, 5}}),
                  to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
              false);
    ASSERT_EQ(a_overlaps_any_of_b(
                  to_selection({part_t {0, 1}, part_t {2, 3}, part_t {4, 5}}),
                  to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
              false);
}

//
// disjoint - PartSelection & PartSelection
//

TEST(GeometryPartSelections, SelectionDisjointOfSelection0) {
    ASSERT_EQ(a_disjoint_of_b(to_selection({}), to_selection({part_t {5, 10}})), true);
    ASSERT_EQ(a_disjoint_of_b(to_selection({part_t {10, 20}}), to_selection({})), true);
    ASSERT_EQ(a_disjoint_of_b(to_selection({}), to_selection({})), true);
}

TEST(GeometryPartSelections, SelectionDisjointOfSelection1) {
    ASSERT_EQ(
        a_disjoint_of_b(to_selection({part_t {3, 6}}), to_selection({part_t {5, 10}})),
        false);
    ASSERT_EQ(
        a_disjoint_of_b(to_selection({part_t {6, 9}}), to_selection({part_t {5, 10}})),
        false);
    ASSERT_EQ(
        a_disjoint_of_b(to_selection({part_t {5, 10}}), to_selection({part_t {5, 10}})),
        false);

    ASSERT_EQ(
        a_disjoint_of_b(to_selection({part_t {0, 5}}), to_selection({part_t {5, 10}})),
        true);
    ASSERT_EQ(
        a_disjoint_of_b(to_selection({part_t {10, 20}}), to_selection({part_t {5, 10}})),
        true);
}

TEST(GeometryPartSelections, SelectionDisjointOfSelection2) {
    ASSERT_EQ(
        a_disjoint_of_b(to_selection({part_t {3, 6}}),
                        to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
        false);
    ASSERT_EQ(
        a_disjoint_of_b(to_selection({part_t {15, 16}}),
                        to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
        false);
    ASSERT_EQ(
        a_disjoint_of_b(to_selection({part_t {29, 30}}),
                        to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
        false);

    ASSERT_EQ(
        a_disjoint_of_b(to_selection({part_t {0, 3}}),
                        to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
        true);
    ASSERT_EQ(
        a_disjoint_of_b(to_selection({part_t {20, 25}}),
                        to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
        true);
    ASSERT_EQ(
        a_disjoint_of_b(to_selection({part_t {35, 40}}),
                        to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
        true);
}

TEST(GeometryPartSelections, SelectionDisjointOfSelection3) {
    ASSERT_EQ(
        a_disjoint_of_b(
            to_selection({part_t {0, 1}, part_t {2, 3}, part_t {4, 5}, part_t {15, 16}}),
            to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
        false);
    ASSERT_EQ(
        a_disjoint_of_b(
            to_selection({part_t {0, 1}, part_t {2, 3}, part_t {4, 5}, part_t {6, 7}}),
            to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
        false);
    ASSERT_EQ(
        a_disjoint_of_b(
            to_selection({part_t {0, 1}, part_t {2, 3}, part_t {4, 5}, part_t {29, 30}}),
            to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
        false);
    ASSERT_EQ(
        a_disjoint_of_b(to_selection({part_t {24, 26}, part_t {40, 41}, part_t {43, 44},
                                      part_t {50, 51}}),
                        to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
        false);

    ASSERT_EQ(
        a_disjoint_of_b(to_selection({part_t {0, 1}, part_t {2, 3}, part_t {4, 5}}),
                        to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
        true);
    ASSERT_EQ(
        a_disjoint_of_b(to_selection({part_t {0, 1}, part_t {2, 3}, part_t {4, 5}}),
                        to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
        true);
    ASSERT_EQ(
        a_disjoint_of_b(to_selection({part_t {0, 1}, part_t {2, 3}, part_t {4, 5}}),
                        to_selection({part_t {5, 10}, part_t {15, 20}, part_t {25, 30}})),
        true);
}

//
// Iter Parts
//

namespace {

using IterPartsResult = std::vector<std::pair<part_t, bool>>;

auto iter_parts_result(part_t full_part, std::initializer_list<part_t> list)
    -> IterPartsResult {
    auto result = IterPartsResult {};

    iter_parts(full_part, to_selection(list), [&](part_t part, bool selected) {
        result.push_back({part, selected});
    });

    return result;
}

}  // namespace

TEST(GeometryPartSelections, IterPartsSelection0) {
    {
        const auto result = iter_parts_result(part_t {0, 100}, {});
        const auto expected = IterPartsResult {
            {part_t {0, 100}, false},
        };

        ASSERT_EQ(result, expected);
    }
}

TEST(GeometryPartSelections, IterPartsSelection1) {
    {
        const auto result = iter_parts_result(part_t {0, 100}, {part_t {10, 20}});
        const auto expected = IterPartsResult {
            {part_t {0, 10}, false},
            {part_t {10, 20}, true},
            {part_t {20, 100}, false},
        };

        ASSERT_EQ(result, expected);
    }

    {
        const auto result = iter_parts_result(part_t {0, 20}, {part_t {10, 20}});
        const auto expected = IterPartsResult {
            {part_t {0, 10}, false},
            {part_t {10, 20}, true},
        };

        ASSERT_EQ(result, expected);
    }

    {
        const auto result = iter_parts_result(part_t {0, 100}, {part_t {0, 10}});
        const auto expected = IterPartsResult {
            {part_t {0, 10}, true},
            {part_t {10, 100}, false},
        };

        ASSERT_EQ(result, expected);
    }

    {
        const auto result = iter_parts_result(part_t {0, 10}, {part_t {0, 10}});
        const auto expected = IterPartsResult {
            {part_t {0, 10}, true},
        };

        ASSERT_EQ(result, expected);
    }
}

TEST(GeometryPartSelections, IterPartsSelection2) {
    {
        const auto result =
            iter_parts_result(part_t {0, 100}, {part_t {10, 20}, part_t {50, 60}});
        const auto expected = IterPartsResult {
            {part_t {0, 10}, false}, {part_t {10, 20}, true},   {part_t {20, 50}, false},
            {part_t {50, 60}, true}, {part_t {60, 100}, false},
        };

        ASSERT_EQ(result, expected);
    }

    {
        const auto result =
            iter_parts_result(part_t {0, 60}, {part_t {10, 20}, part_t {50, 60}});
        const auto expected = IterPartsResult {
            {part_t {0, 10}, false},
            {part_t {10, 20}, true},
            {part_t {20, 50}, false},
            {part_t {50, 60}, true},
        };

        ASSERT_EQ(result, expected);
    }

    {
        const auto result =
            iter_parts_result(part_t {0, 60}, {part_t {0, 20}, part_t {50, 60}});
        const auto expected = IterPartsResult {
            {part_t {0, 20}, true},
            {part_t {20, 50}, false},
            {part_t {50, 60}, true},
        };

        ASSERT_EQ(result, expected);
    }
}

//
// Iterate overlapping parts
//

namespace {

using IterOverlappingResult = std::vector<std::tuple<part_t, part_t, bool>>;

auto iter_overlapping_result(part_t full_part, std::initializer_list<part_t> query,
                             std::initializer_list<part_t> target)
    -> IterOverlappingResult {
    auto result = IterOverlappingResult {};

    iter_overlapping_parts(
        full_part, to_selection(query), to_selection(target),
        [&](part_t query_part, part_t target_part, bool target_selected) {
            result.push_back({query_part, target_part, target_selected});
        });

    return result;
}

}  // namespace

TEST(GeometryPartSelections, IterOverlappingParts0) {
    {
        const auto result = iter_overlapping_result(part_t {0, 100}, {}, {});
        const auto expected = IterOverlappingResult {};

        ASSERT_EQ(result, expected);
    }

    {
        const auto result =
            iter_overlapping_result(part_t {0, 100}, {part_t {50, 60}}, {});
        const auto expected = IterOverlappingResult {
            {part_t {50, 60}, part_t {0, 100}, false},
        };

        ASSERT_EQ(result, expected);
    }

    {
        const auto result =
            iter_overlapping_result(part_t {0, 100}, {}, {part_t {50, 60}});
        const auto expected = IterOverlappingResult {};

        ASSERT_EQ(result, expected);
    }
}

TEST(GeometryPartSelections, IterOverlappingParts1) {
    {
        const auto result = iter_overlapping_result(part_t {0, 100}, {part_t {10, 20}},
                                                    {part_t {50, 60}});
        const auto expected = IterOverlappingResult {
            {part_t {10, 20}, part_t {0, 50}, false},
        };

        ASSERT_EQ(result, expected);
    }

    {
        const auto result = iter_overlapping_result(part_t {0, 100}, {part_t {55, 56}},
                                                    {part_t {50, 60}});
        const auto expected = IterOverlappingResult {
            {part_t {55, 56}, part_t {50, 60}, true},
        };

        ASSERT_EQ(result, expected);
    }

    {
        const auto result = iter_overlapping_result(part_t {0, 100}, {part_t {10, 90}},
                                                    {part_t {50, 60}});
        const auto expected = IterOverlappingResult {
            {part_t {10, 90}, part_t {0, 50}, false},
            {part_t {10, 90}, part_t {50, 60}, true},
            {part_t {10, 90}, part_t {60, 100}, false},
        };

        ASSERT_EQ(result, expected);
    }
}

TEST(GeometryPartSelections, IterOverlappingParts2) {
    {
        const auto result = iter_overlapping_result(part_t {0, 100}, {part_t {10, 90}},
                                                    {part_t {30, 40}, part_t {60, 70}});
        const auto expected = IterOverlappingResult {
            {part_t {10, 90}, part_t {0, 30}, false},
            {part_t {10, 90}, part_t {30, 40}, true},
            {part_t {10, 90}, part_t {40, 60}, false},
            {part_t {10, 90}, part_t {60, 70}, true},
            {part_t {10, 90}, part_t {70, 100}, false},
        };

        ASSERT_EQ(result, expected);
    }

    {
        const auto result =
            iter_overlapping_result(part_t {0, 100}, {part_t {10, 45}, part_t {50, 65}},
                                    {part_t {30, 40}, part_t {60, 70}});
        const auto expected = IterOverlappingResult {
            {part_t {10, 45}, part_t {0, 30}, false},
            {part_t {10, 45}, part_t {30, 40}, true},
            {part_t {10, 45}, part_t {40, 60}, false},
            {part_t {50, 65}, part_t {40, 60}, false},
            {part_t {50, 65}, part_t {60, 70}, true},
        };

        ASSERT_EQ(result, expected);
    }

    {
        const auto result = iter_overlapping_result(
            part_t {0, 100},
            {part_t {20, 30}, part_t {35, 40}, part_t {45, 50}, part_t {55, 60}},
            {part_t {10, 80}});
        const auto expected = IterOverlappingResult {
            {part_t {20, 30}, part_t {10, 80}, true},
            {part_t {35, 40}, part_t {10, 80}, true},
            {part_t {45, 50}, part_t {10, 80}, true},
            {part_t {55, 60}, part_t {10, 80}, true},
        };

        ASSERT_EQ(result, expected);
    }

    {
        const auto result =
            iter_overlapping_result(part_t {0, 100}, {part_t {10, 20}, part_t {30, 40}},
                                    {part_t {10, 20}, part_t {30, 40}});
        const auto expected = IterOverlappingResult {
            {part_t {10, 20}, part_t {10, 20}, true},
            {part_t {30, 40}, part_t {30, 40}, true},
        };

        ASSERT_EQ(result, expected);
    }

    {
        const auto result = iter_overlapping_result(
            part_t {0, 100}, {part_t {0, 10}, part_t {20, 30}, part_t {40, 100}},
            {part_t {10, 20}, part_t {30, 40}});
        const auto expected = IterOverlappingResult {
            {part_t {0, 10}, part_t {0, 10}, false},
            {part_t {20, 30}, part_t {20, 30}, false},
            {part_t {40, 100}, part_t {40, 100}, false},
        };

        ASSERT_EQ(result, expected);
    }
}

}  // namespace logicsim
