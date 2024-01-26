#ifndef LOGICSIM_GEOMETRY_PART_SELECTIONS_H
#define LOGICSIM_GEOMETRY_PART_SELECTIONS_H

#include "part_selection.h"
#include "vocabulary/part.h"

namespace logicsim {

[[nodiscard]] auto a_overlaps_any_of_b(part_t a, const PartSelection& b) -> bool;

[[nodiscard]] auto a_overlaps_any_of_b(const PartSelection& a, const PartSelection& b)
    -> bool;
[[nodiscard]] auto a_disjoint_of_b(const PartSelection& a, const PartSelection& b)
    -> bool;

/**
 * @brief: Iterator over selected and unselected parts.
 *
 * [&](part_t part, bool selected){}
 */
template <typename Func>
auto iter_parts(part_t full_part, const PartSelection& parts, Func func) {
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
}

/**
 * @brief:
 *
 * [&](part_t query_part, part_t target_part, bool target_selected){}
 */
template <typename Func>
auto iter_overlapping_parts(part_t full_part, const PartSelection& query,
                            const PartSelection& target, Func func) {
    auto it = query.begin();
    const auto end = query.end();

    iter_parts(full_part, target, [&](part_t target_part, bool target_selected) {
        while (it != end && it->end <= target_part.begin) {
            ++it;
        }
        if (it != end) {
            func(*it, target_part, target_selected);
        }
    });
}

}  // namespace logicsim

#endif
