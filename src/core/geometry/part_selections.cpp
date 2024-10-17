#include "core/geometry/part_selections.h"

#include "core/algorithm/merged_any_of.h"
#include "core/algorithm/merged_none_of.h"
#include "core/geometry/part.h"
#include "core/part_selection.h"
#include "core/vocabulary/part.h"

namespace logicsim {

auto a_overlaps_any_of_b(part_t a, const PartSelection& b) -> bool {
    const auto i1 = std::ranges::upper_bound(b, a.begin, {},
                                             [](const part_t& part) { return part.end; });
    const auto i2 = std::ranges::upper_bound(
        b, a.end, {}, [](const part_t& part) { return part.begin; });

    return std::ranges::any_of(
        i1, i2, [a](const part_t& part) { return a_overlaps_any_of_b(a, part); });
}

auto a_overlaps_any_of_b(const PartSelection& a, const PartSelection& b) -> bool {
    return merged_any_of(a, b, {}, [](const part_t& a_, const part_t& b_) {
        return a_overlaps_any_of_b(a_, b_);
    });
}

auto a_disjoint_b(const PartSelection& a, const PartSelection& b) -> bool {
    return merged_none_of(a, b, {}, [](const part_t& a_, const part_t& b_) {
        return a_overlaps_any_of_b(a_, b_);
    });
}

}  // namespace logicsim
