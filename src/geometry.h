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

// fast distance for horitonal or vertical lines
auto distance(line_t line) -> int;

auto is_endpoint(point_t point, line_t line) -> bool;

//
// orientation_t
//

auto to_orientation(point_t p0, point_t p1) -> orientation_t;
auto to_orientation(line_t line) -> orientation_t;

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

// TODO rename: to_part, to_line
auto to_part(ordered_line_t line) -> part_t;
auto to_part(ordered_line_t line, rect_fine_t rect) -> std::optional<part_t>;
auto to_line(ordered_line_t line, part_t part) -> ordered_line_t;

//
// Parts List
//

[[nodiscard]] auto is_part_inside_line(part_t part, ordered_line_t line) -> bool;
auto sort_and_validate_segment_parts(std::span<part_t> parts, ordered_line_t line)
    -> void;
auto validate_segment_parts(std::span<const part_t> parts, ordered_line_t line) -> void;

[[nodiscard]] auto a_inside_b_not_touching(part_t a, part_t b) -> bool;
[[nodiscard]] auto a_disjoint_to_b(part_t a, part_t b) -> bool;
[[nodiscard]] auto a_inside_b_touching_one_side(part_t a, part_t b) -> bool;
[[nodiscard]] auto a_equal_b(part_t a, part_t b) -> bool;

// TODO remove this and use named methods instead?
enum class InclusionResult {
    fully_included,
    not_included,
    partially_overlapping,
};
[[nodiscard]] auto format(InclusionResult state) -> std::string;

[[nodiscard]] auto a_part_of_b(part_t a, part_t b) -> InclusionResult;
[[nodiscard]] auto is_part_included(std::span<const part_t> parts, part_t part)
    -> InclusionResult;

template <typename Container = std::vector<part_t>>
auto add_part(Container &entries, part_t new_part) -> void {
    entries.push_back(new_part);
    std::ranges::sort(entries);

    // merge elements
    auto result = Container {};
    using it_t = Container::iterator;

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
    // entries.swap(result);
}

template <typename Container = std::vector<part_t>>
auto remove_part(Container &entries, part_t removing) -> void {
    for (auto i : reverse_range(entries.size())) {
        const auto entry = part_t {entries[i]};

        // SEE 'selection_model.md' for visual cases

        // no overlapp -> keep
        // TODO use named methods
        if (entry.begin >= removing.end || entry.end <= removing.begin) {
        }

        // new completely inside -> split
        else if (entry.begin < removing.begin && entry.end > removing.end) {
            entries[i] = part_t {entry.begin, removing.begin};
            entries.emplace_back(removing.end, entry.end);
        }

        // new complete overlapps -> swap & remove
        else if (entry.begin >= removing.begin && entry.end <= removing.end) {
            entries[i] = entries[entries.size() - 1];
            entries.pop_back();
        }

        // right sided overlap -> shrink right
        else if (entry.begin < removing.begin && entry.end > removing.begin
                 && entry.end <= removing.end) {
            entries[i] = part_t {entry.begin, removing.begin};
        }
        // left sided overlap -> shrink left
        else if (entry.begin >= removing.begin && entry.begin < removing.end
                 && entry.end > removing.end) {
            entries[i] = part_t {removing.end, entry.end};
        }

        else {
            throw_exception("unknown case in remove_segment");
        }
    }
}

}  // namespace logicsim

//
// Formatters
//

template <>
struct fmt::formatter<logicsim::InclusionResult> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::InclusionResult &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", ::logicsim::format(obj));
    }
};

#endif