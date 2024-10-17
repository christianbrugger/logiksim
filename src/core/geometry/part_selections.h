#ifndef LOGICSIM_GEOMETRY_PART_SELECTIONS_H
#define LOGICSIM_GEOMETRY_PART_SELECTIONS_H

#include "core/geometry/part.h"
#include "core/part_selection.h"
#include "core/vocabulary/part.h"

namespace logicsim {

[[nodiscard]] auto a_overlaps_any_of_b(part_t a, const PartSelection& b) -> bool;

[[nodiscard]] auto a_overlaps_any_of_b(const PartSelection& a,
                                       const PartSelection& b) -> bool;
[[nodiscard]] auto a_disjoint_b(const PartSelection& a, const PartSelection& b) -> bool;

/**
 * @brief: Iterator over selected and unselected parts.
 *
 * [&](part_t part, bool selected){}
 */
template <typename Func>
auto iter_parts(part_t full_part, const PartSelection& parts, Func func) {
    Expects(full_part.begin == offset_t {0});

    offset_t pivot = full_part.begin;

    for (const auto& part : parts) {
        if (pivot != part.begin) {
            func(part_t {pivot, part.begin}, false);
        }
        func(part, true);
        pivot = part.end;
    }

    if (pivot != full_part.end) {
        func(part_t {pivot, full_part.end}, false);
    }

    Ensures(pivot <= full_part.end);
}

/**
 * @brief: Iterates over query and selected part and calls function for each
 *         overlapping query part with corresponding selected & unselected
 *         target parts.
 *
 * [&](part_t query_part, part_t target_part, bool target_selected){}
 */
template <typename Func>
auto iter_overlapping_parts(part_t full_part, const PartSelection& query,
                            const PartSelection& target, Func func) {
    auto pivot = query.begin();
    const auto end = query.end();

    if (pivot == end) {
        return;
    }

    iter_parts(full_part, target, [&](part_t target_part, bool target_selected) {
        Expects(pivot == end || pivot->end > target_part.begin);

        while (pivot != end && pivot->end <= target_part.end) {
            Expects(a_overlaps_any_of_b(*pivot, target_part));
            func(*pivot, target_part, target_selected);
            ++pivot;
        }
        if (pivot != end && pivot->begin < target_part.end) {
            Expects(a_overlaps_any_of_b(*pivot, target_part));
            func(*pivot, target_part, target_selected);
        }
    });

    Ensures(pivot == end);
}

}  // namespace logicsim

#endif
