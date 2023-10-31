#include "part_selection.h"

#include "algorithm/range.h"
#include "algorithm/transform_combine_while.h"
#include "allocated_size/folly_small_vector.h"
#include "format/container.h"
#include "geometry/part.h"
#include "part_selection.h"
#include "range/v3/view/drop.hpp"
#include "range/v3/view/zip.hpp"
#include "vocabulary/grid.h"

#include <fmt/core.h>

#include <stdexcept>

namespace logicsim {

namespace part_selection {

namespace {

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

#ifndef NDEBUG
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
#endif

}  // namespace

}  // namespace part_selection

auto PartSelection::format() const -> std::string {
    return fmt::format("<part-selection: {}>", parts_);
}

auto PartSelection::begin() const -> iterator {
    return parts_.begin();
}

auto PartSelection::end() const -> iterator {
    return parts_.end();
}

auto PartSelection::front() const -> part_t {
    return parts_.front();
}

auto PartSelection::back() const -> part_t {
    return parts_.back();
}

auto PartSelection::max_offset() const -> offset_t {
    if (empty()) {
        return offset_t {0};
    }
    return parts_.back().end;
}

PartSelection::PartSelection(part_t part) : parts_ {part} {}

PartSelection::PartSelection(part_vector_t&& parts) : parts_ {std::move(parts)} {
    part_selection::sort_and_merge_parts(parts_);

    assert(std::ranges::is_sorted(parts_));
    assert(part_selection::parts_not_touching(parts_));
}

auto PartSelection::inverted(const PartSelection& source, part_t part) -> PartSelection {
    if (source.empty()) {
        return PartSelection {part};
    }

    auto result = PartSelection {};
    const auto add_if_positive = [&](offset_t begin, offset_t end) -> void {
        if (begin < end) {
            result.parts_.push_back(part_t {begin, end});
        }
    };

    add_if_positive(part.begin, source.front().begin);
    for (const auto& [part1, part2] :
         ranges::views::zip(source, source | ranges::views::drop(1))) {
        add_if_positive(std::max(part.begin, part1.end),  //
                        std::min(part2.end, part.end));
    }
    add_if_positive(source.back().end, part.end);

    assert(std::ranges::is_sorted(result.parts_));
    assert(part_selection::parts_not_touching(result.parts_));
    return result;
}

auto PartSelection::empty() const noexcept -> bool {
    return parts_.empty();
}

auto PartSelection::size() const noexcept -> std::size_t {
    return parts_.size();
}

auto PartSelection::allocated_size() const -> std::size_t {
    return get_allocated_size(parts_);
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

namespace part_selection {

namespace {

[[nodiscard]] auto get_shifted_part(part_t part, offset_t::difference_type shifted,
                                    offset_t::difference_type max_end)
    -> std::optional<part_t> {
    using V = offset_t::difference_type;

    const auto begin = V {part.begin.value} + shifted;
    const auto end = std::min(V {part.end.value} + shifted, max_end);

    if (begin < end) {
        return part_t {offset_t {gsl::narrow_cast<offset_t::value_type>(begin)},
                       offset_t {gsl::narrow_cast<offset_t::value_type>(end)}};
    }
    return std::nullopt;
}

auto copy_parts(part_vector_t& destination, const part_vector_t& source,
                part_copy_definition_t def) {
    if (distance(def.destination) != distance(def.source)) {
        throw std::runtime_error("source and destination need to have the same size");
    }

    using V = offset_t::difference_type;
    auto shifted = V {def.destination.begin.value} - V {def.source.begin.value};
    auto max_end = V {def.destination.end.value};

    for (const part_t& part : source) {
        if (const std::optional<part_t> res = intersect(part, def.source)) {
            if (const auto new_part = get_shifted_part(*res, shifted, max_end)) {
                assert(a_inside_b(*new_part, def.destination));
                destination.push_back(*new_part);
            }
        }
    }
}

}  // namespace

}  // namespace part_selection

auto PartSelection::copy_parts(const PartSelection& source,
                               part_copy_definition_t copy_definition) -> void {
    assert(std::ranges::is_sorted(parts_));
    assert(part_selection::parts_not_touching(parts_));

    const bool original_empty = empty();
    part_selection::copy_parts(parts_, source.parts_, copy_definition);
    if (!original_empty) {
        part_selection::sort_and_merge_parts(parts_);
    }

    assert(std::ranges::is_sorted(parts_));
    assert(part_selection::parts_not_touching(parts_));
}

//
// Free functions
//

auto copy_parts(const PartSelection& source, part_copy_definition_t copy_definition)
    -> PartSelection {
    auto result = PartSelection {};
    result.copy_parts(source, copy_definition);
    return result;
}

auto move_parts(move_definition_t attrs) -> void {
    if (&attrs.source == &attrs.destination) [[unlikely]] {
        throw std::runtime_error("Source and destination need to be independent.");
    }

    attrs.destination.copy_parts(attrs.source, attrs.copy_definition);
    attrs.source.remove_part(attrs.copy_definition.source);
}

auto move_parts(PartSelection& parts, part_copy_definition_t copy_definition) -> void {
    auto result = PartSelection {parts};
    result.remove_part(copy_definition.source);
    result.copy_parts(parts, copy_definition);
    parts = result;
}

}  // namespace logicsim
