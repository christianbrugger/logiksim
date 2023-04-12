#ifndef LOGIKSIM_GEOMETRY_H
#define LOGIKSIM_GEOMETRY_H

#include "vocabulary.h"

#include <tuple>

namespace logicsim {

//
// line_t
//

auto is_horizontal(line_t line) noexcept -> bool;
auto is_vertical(line_t line) noexcept -> bool;

auto is_horizontal(ordered_line_t line) noexcept -> bool;
auto is_vertical(ordered_line_t line) noexcept -> bool;

// order points within lines
// auto order_points(line_t line) noexcept -> line_t;
// order lines and points within lines
// auto order_points(const line_t line0, const line_t line1) noexcept
//    -> std::tuple<line_t, line_t>;
auto order_points(const line_t line0, const line_t line1) noexcept
    -> std::tuple<ordered_line_t, ordered_line_t>;

auto distance(part_t part) -> int;
auto distance(line_t line) -> int;
auto distance(ordered_line_t line) -> int;

auto is_endpoint(point_t point, line_t line) -> bool;
auto is_endpoint(point_t point, ordered_line_t line) -> bool;

auto add_unchecked(ordered_line_t line, int dx, int dy) -> ordered_line_t;

//
// orientation_t
//

// from p0 to p1
auto to_orientation(point_t p0, point_t p1) -> orientation_t;

// angle in respect to left orientation
auto to_angle(orientation_t orientation) -> double;

//
// offset_t
//

auto to_offset(grid_t x, grid_t reference) -> offset_t;
auto to_grid(offset_t offset, grid_t reference) -> grid_t;

//
// part_t
//

[[nodiscard]] auto a_inside_b(part_t a, part_t b) -> bool;
[[nodiscard]] auto a_inside_b_not_touching(part_t a, part_t b) -> bool;
[[nodiscard]] auto a_inside_b_touching_one_side(part_t a, part_t b) -> bool;
[[nodiscard]] auto a_inside_b_touching_begin(part_t a, part_t b) -> bool;
[[nodiscard]] auto a_inside_b_touching_end(part_t a, part_t b) -> bool;
[[nodiscard]] auto a_disjoint_b(part_t a, part_t b) -> bool;
[[nodiscard]] auto a_equal_b(part_t a, part_t b) -> bool;
[[nodiscard]] auto a_overlapps_any_of_b(part_t a, part_t b) -> bool;
[[nodiscard]] auto a_overlapps_b_begin(part_t a, part_t b) -> bool;
[[nodiscard]] auto a_overlapps_b_end(part_t a, part_t b) -> bool;

[[nodiscard]] auto to_part(ordered_line_t full_line) -> part_t;
[[nodiscard]] auto to_part(ordered_line_t full_line, rect_fine_t rect)
    -> std::optional<part_t>;
[[nodiscard]] auto to_part(ordered_line_t full_line, ordered_line_t line) -> part_t;
[[nodiscard]] auto to_line(ordered_line_t full_line, part_t part) -> ordered_line_t;

[[nodiscard]] auto is_part_valid(part_t part, ordered_line_t full_line) -> bool;
[[nodiscard]] auto to_segment_part(segment_t segment, ordered_line_t sub_line)
    -> segment_part_t;

[[nodiscard]] auto intersect(part_t a, part_t b) -> std::optional<part_t>;
[[nodiscard]] auto difference_touching_one_side(part_t full_part, part_t b) -> part_t;
[[nodiscard]] auto difference_not_touching(part_t full_part, part_t b)
    -> std::pair<part_t, part_t>;

//
// Parts List
//

auto sort_and_validate_segment_parts(std::span<part_t> parts, ordered_line_t line)
    -> void;
auto validate_segment_parts(std::span<const part_t> parts, ordered_line_t line) -> void;

template <typename Container = std::vector<part_t>>
auto _sort_and_merge_parts(Container &entries) -> void {
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
        throw_exception("algorithm result should not be empty");
    }

    using std::swap;
    swap(entries, result);
}

template <typename Container = std::vector<part_t>>
auto add_part(Container &entries, part_t new_part) -> void {
    entries.push_back(new_part);
    _sort_and_merge_parts(entries);
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
            throw_exception("unknown case in remove_segment");
        }
    }
}

//
// part copying
//

template <typename V = offset_t::difference_type>
auto _get_shifted_part(part_t part, V shifted, V max_end) -> std::optional<part_t> {
    const auto begin = V {part.begin.value} + shifted;
    const auto end = std::min(V {part.end.value} + shifted, max_end);

    if (begin < end) {
        return part_t {offset_t {gsl::narrow_cast<offset_t::value_type>(begin)},
                       offset_t {gsl::narrow_cast<offset_t::value_type>(end)}};
    }
    return std::nullopt;
}

template <typename Container = std::vector<part_t>>
auto _add_intersecting_parts(const Container &source_entries,
                             Container &destination_entries, part_t part_destination) {
    using V = offset_t::difference_type;

    auto shifted = V {part_destination.begin.value};
    auto max_end = V {part_destination.end.value};

    for (const part_t part : source_entries) {
        if (const auto new_part = _get_shifted_part<V>(part, shifted, max_end)) {
            assert(a_inside_b(*new_part, part_destination));
            destination_entries.push_back(*new_part);
        }
    }
}

template <typename Container = std::vector<part_t>>
[[nodiscard]] auto copy_parts(const Container &source_entries, part_t part_destination)
    -> Container {
    auto result = Container {};
    _add_intersecting_parts(source_entries, result, part_destination);
    return result;
}

template <typename Container = std::vector<part_t>>
auto copy_parts(const Container &source_entries, Container &destination_entries,
                part_t part_destination) -> void {
    bool original_empty = destination_entries.empty();

    _add_intersecting_parts(source_entries, destination_entries, part_destination);

    if (!original_empty) {
        _sort_and_merge_parts(destination_entries);
    }
}

template <typename Container = std::vector<part_t>>
auto _add_intersecting_parts(const Container &source_entries,
                             Container &destination_entries,
                             part_copy_definition_t parts) {
    if (distance(parts.destination) != distance(parts.source)) {
        throw_exception("source and destination need to have the same size");
    }

    using V = offset_t::difference_type;

    auto shifted = V {parts.destination.begin.value} - V {parts.source.begin.value};
    auto max_end = V {parts.destination.end.value};

    for (const part_t part : source_entries) {
        if (const std::optional<part_t> res = intersect(part, parts.source)) {
            if (const auto new_part = _get_shifted_part<V>(*res, shifted, max_end)) {
                assert(a_inside_b(*new_part, parts.destination));
                destination_entries.push_back(*new_part);
            }
        }
    }
}

template <typename Container = std::vector<part_t>>
[[nodiscard]] auto copy_parts(const Container &source_entries,
                              part_copy_definition_t parts) -> Container {
    auto result = Container {};
    _add_intersecting_parts(source_entries, result, parts);
    return result;
}

template <typename Container = std::vector<part_t>>
auto copy_parts(const Container &source_entries, Container &destination_entries,
                part_copy_definition_t parts) -> void {
    bool original_empty = destination_entries.empty();

    _add_intersecting_parts(source_entries, destination_entries, parts);

    if (!original_empty) {
        _sort_and_merge_parts(destination_entries);
    }
}

template <typename Container = std::vector<part_t>>
auto move_parts(Container &source_entries, Container &destination_entries,
                part_copy_definition_t parts) -> void {
    copy_parts(source_entries, destination_entries, parts);
    remove_part(source_entries, parts.source);
}

}  // namespace logicsim

#endif