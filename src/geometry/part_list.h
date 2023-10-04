#ifndef LOGICSIM_GEOMETRY_PART_LIST_H
#define LOGICSIM_GEOMETRY_PART_LIST_H

#include "algorithm/range.h"
#include "algorithm/transform_combine_while.h"
#include "geometry/part.h"
#include "vocabulary/ordered_line.h"
#include "vocabulary/part.h"

#include <algorithm>
#include <span>
#include <stdexcept>
#include <vector>

namespace logicsim {

auto sort_and_validate_segment_parts(std::span<part_t> parts, ordered_line_t line)
    -> void;
auto validate_segment_parts(std::span<const part_t> parts, ordered_line_t line) -> void;

template <typename Container = std::vector<part_t>>
auto sort_and_merge_parts(Container &entries) -> void {
    if (entries.empty()) {
        return;
    }
    std::ranges::sort(entries);

    // merge elements
    auto result = Container {};
    using it_t = typename Container::iterator;

    transform_combine_while(
        entries, std::back_inserter(result),
        // make state
        [](it_t it) -> part_t { return *it; },
        // combine while
        [](part_t state, it_t it) -> bool { return state.end >= it->begin; },
        // update state
        [](part_t state, it_t it) -> part_t {
            return part_t {state.begin, std::max(state.end, it->end)};
        });

    if (result.size() == 0) [[unlikely]] {
        std::runtime_error("algorithm result should not be empty");
    }

    using std::swap;
    swap(entries, result);
}

template <typename Container = std::vector<part_t>>
auto add_part(Container &entries, part_t new_part) -> void {
    entries.push_back(new_part);
    sort_and_merge_parts(entries);
}

template <typename Container = std::vector<part_t>>
auto remove_part(Container &entries, part_t removing) -> void {
    for (auto i : reverse_range(entries.size())) {
        const auto entry = part_t {entries[i]};

        // See selection_model.md

        // no overlapp -> keep
        if (a_disjoint_b(removing, entry)) {
        }

        // new completely inside -> split
        else if (a_inside_b_not_touching(removing, entry)) {
            entries[i] = part_t {entry.begin, removing.begin};
            entries.emplace_back(removing.end, entry.end);
        }

        // new complete overlapps -> swap & remove
        else if (a_inside_b(entry, removing)) {
            entries[i] = entries[entries.size() - 1];
            entries.pop_back();
        }

        // begin overlap -> shrink begin
        else if (a_overlapps_b_begin(removing, entry)) {
            entries[i] = part_t {removing.end, entry.end};
        }
        // end overlap -> shrink end
        else if (a_overlapps_b_end(removing, entry)) {
            entries[i] = part_t {entry.begin, removing.begin};
        }

        else {
            std::runtime_error("unknown case in remove_segment");
        }
    }
}

}  // namespace logicsim

#endif
