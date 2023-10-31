#include "part_selection.h"

#include "algorithm/transform_combine_while.h"
#include "format/container.h"
#include "vocabulary/grid.h"
#include "algorithm/range.h"
#include "geometry/part.h"

#include <fmt/core.h>
#include <stdexcept>

namespace logicsim {

namespace part_selection {

auto sort_and_merge_parts(part_vector_t& parts) -> void {
    if (parts.empty()) {
        return;
    }
    std::ranges::sort(parts);

    // merge elements
    using it_t = typename part_vector_t::iterator;
    const auto it = transform_combine_while(
        parts, std::begin(parts),
        // make state
        [](it_t it) -> part_t { return *it; },
        // combine while
        [](part_t state, it_t it) -> bool { return state.end >= it->begin; },
        // update state
        [](part_t state, it_t it) -> part_t {
            return part_t {state.begin, std::max(state.end, it->end)};
        });
    parts.erase(it, parts.end());

    Ensures(!parts.empty());
}

/**
 * @brief: Returns false if parts are overlapping or touching.
 */
[[nodiscard]] auto parts_not_touching(const part_vector_t& parts) -> bool {
    Expects(std::ranges::is_sorted(parts));

    const auto part_overlapping = [](part_t part0, part_t part1) -> bool {
        return part0.end >= part1.begin;
    };
    return std::ranges::adjacent_find(parts, part_overlapping) == parts.end();
}

}  // namespace part_selection

auto PartSelection::format() const -> std::string {
    return fmt::format("{}", "<part_selection>");
}

auto PartSelection::add_part(part_t part) -> void {
    assert(std::ranges::is_sorted(parts_));
    assert(part_selection::parts_not_touching(parts_));

    parts_.push_back(part);
    part_selection::sort_and_merge_parts(parts_);

    assert(std::ranges::is_sorted(parts_));
    assert(part_selection::parts_not_touching(parts_));
}

auto PartSelection::remove_part(part_t removing) -> void {
    assert(std::ranges::is_sorted(parts_));
    assert(part_selection::parts_not_touching(parts_));

    bool require_sort = false;

    for (auto i : reverse_range(parts_.size())) {
        assert(0 <= i && i < parts_.size());
        const auto part = part_t {parts_[i]};

        // See selection_model.md

        // no overlap -> keep
        if (a_disjoint_b(removing, part)) {
        }

        // new completely inside -> split
        else if (a_inside_b_not_touching(removing, part)) {
            parts_[i] = part_t {part.begin, removing.begin};
            parts_.emplace_back(removing.end, part.end);
            require_sort = true;
        }

        // new complete overlaps -> swap & remove
        else if (a_inside_b(part, removing)) {
            parts_[i] = parts_.back();
            parts_.pop_back();
            require_sort = true;
        }

        // begin overlap -> shrink begin
        else if (a_overlapps_b_begin(removing, part)) {
            parts_[i] = part_t {removing.end, part.end};
        }
        // end overlap -> shrink end
        else if (a_overlapps_b_end(removing, part)) {
            parts_[i] = part_t {part.begin, removing.begin};
        }

        else {
            // unknown case
            throw std::runtime_error("unknown case in remove_segment");
        }
    }

    if (require_sort) {
        std::ranges::sort(parts_);
    }

    assert(std::ranges::is_sorted(parts_));
    assert(part_selection::parts_not_touching(parts_));
}

}  // namespace logicsim
