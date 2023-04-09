#include "geometry.h"

#include "algorithm.h"
#include "exceptions.h"
#include "vocabulary.h"

#include <numbers>

namespace logicsim {

auto is_horizontal(line_t line) noexcept -> bool {
    return line.p0.y == line.p1.y;
}

auto is_vertical(line_t line) noexcept -> bool {
    return line.p0.x == line.p1.x;
}

auto is_horizontal(ordered_line_t line) noexcept -> bool {
    return is_horizontal(line_t {line});
}

auto is_vertical(ordered_line_t line) noexcept -> bool {
    return is_vertical(line_t {line});
}

// order lines and points within lines
auto order_points(const line_t line0, const line_t line1) noexcept
    -> std::tuple<ordered_line_t, ordered_line_t> {
    auto a = ordered_line_t {line0};
    auto b = ordered_line_t {line1};

    if (a.p0 <= b.p0) {
        return std::make_tuple(a, b);
    }
    return std::make_tuple(b, a);
}

auto distance(line_t line) -> int {
    auto dx = line.p1.x.value - line.p0.x.value;
    auto dy = line.p1.y.value - line.p0.y.value;

    // ensure enough precision through promotion
    static_assert(sizeof(dx) > sizeof(line.p0.x.value));
    return std::abs(dx == 0 ? dy : dx);
}

auto distance(ordered_line_t line) -> int {
    auto dx = line.p1.x.value - line.p0.x.value;
    auto dy = line.p1.y.value - line.p0.y.value;

    // ensure enough precision through promotion
    static_assert(sizeof(dx) > sizeof(line.p0.x.value));
    return dx == 0 ? dy : dx;
}

auto to_orientation(point_t p0, point_t p1) -> orientation_t {
    using enum orientation_t;

    if (p1.x > p0.x) {
        return orientation_t::right;
    }
    if (p1.x < p0.x) {
        return orientation_t::left;
    }
    if (p1.y < p0.y) {
        return orientation_t::up;
    }
    if (p1.y > p0.y) {
        return orientation_t::down;
    }

    throw_exception("unexpected case");
}

auto to_angle(orientation_t orientation) -> double {
    switch (orientation) {
        using enum orientation_t;
        case left:
            return 0.;
        case right:
            return std::numbers::pi;
        case up:
            return std::numbers::pi / 2;
        case down:
            return std::numbers::pi * 3 / 2;
        case undirected:
            throw_exception("can't draw arrow for undirected orientation");
    };
    throw_exception("unhandeled orientation");
}

auto is_endpoint(point_t point, line_t line) -> bool {
    return line.p0 == point || line.p1 == point;
}

auto is_endpoint(point_t point, ordered_line_t line) -> bool {
    return is_endpoint(point, line_t {line});
}

//
// offset_t
//

auto to_offset(grid_t x, grid_t reference) -> offset_t {
    const auto value = x.value - reference.value;

    static_assert(sizeof(value) > sizeof(grid_t::value_type));
    static_assert(std::is_signed_v<decltype(value)>);

    return offset_t {gsl::narrow<offset_t::value_type>(value)};
}

auto to_grid(offset_t offset, grid_t reference) -> grid_t {
    const auto value = reference.value + offset.value;

    static_assert(sizeof(value) > sizeof(grid_t::value_type));
    static_assert(std::is_signed_v<decltype(value)>);

    return grid_t {gsl::narrow<grid_t::value_type>(value)};
}

//
// part_t
//

auto to_part(ordered_line_t line) -> part_t {
    if (is_horizontal(line)) {
        const auto end = to_offset(line.p1.x, line.p0.x);
        return part_t {offset_t {0}, end};
    }
    const auto end = to_offset(line.p1.y, line.p0.y);
    return part_t {offset_t {0}, end};
}

auto get_segment_reference_begin_end(ordered_line_t line, rect_fine_t rect) {
    if (is_horizontal(line)) {
        // horizontal
        const auto xmin = clamp_to<grid_t::value_type>(std::floor(rect.p0.x));
        const auto xmax = clamp_to<grid_t::value_type>(std::ceil(rect.p1.x));

        const auto begin = std::clamp(line.p0.x.value, xmin, xmax);
        const auto end = std::clamp(line.p1.x.value, xmin, xmax);

        return std::make_tuple(line.p0.x.value, begin, end);
    }

    // vertical
    const auto ymin = clamp_to<grid_t::value_type>(std::floor(rect.p0.y));
    const auto ymax = clamp_to<grid_t::value_type>(std::ceil(rect.p1.y));

    const auto begin = std::clamp(line.p0.y.value, ymin, ymax);
    const auto end = std::clamp(line.p1.y.value, ymin, ymax);

    return std::make_tuple(line.p0.y.value, begin, end);
}

auto to_part(ordered_line_t line, rect_fine_t rect) -> std::optional<part_t> {
    const auto [reference, begin, end] = get_segment_reference_begin_end(line, rect);

    if (begin == end) {
        return std::nullopt;
    }
    return part_t {to_offset(begin, reference), to_offset(end, reference)};
}

auto to_part(ordered_line_t full_line, ordered_line_t sub_line) -> part_t {
    const auto begin = full_line.p0 == sub_line.p0
                           ? offset_t {0}
                           : to_part(ordered_line_t {full_line.p0, sub_line.p0}).end;
    const auto end = to_part(ordered_line_t {full_line.p0, sub_line.p1}).end;
    const auto full_end = to_part(full_line).end;

    if (end > full_end) [[unlikely]] {
        throw_exception("sub_line needs to be within line");
    }

    return part_t {begin, end};
}

auto to_line(ordered_line_t line, part_t part) -> ordered_line_t {
    const auto x = line.p0.x;
    const auto y = line.p0.y;

    if (is_horizontal(line)) {
        // horizontal
        return ordered_line_t {point_t {to_grid(part.begin, x), y},
                               point_t {to_grid(part.end, x), y}};
    }

    // vertical
    return ordered_line_t {point_t {x, to_grid(part.begin, y)},
                           point_t {x, to_grid(part.end, y)}};
}

auto to_segment_part(segment_t segment, ordered_line_t line) -> segment_part_t {
    return segment_part_t {segment, to_part(line)};
}

auto intersect(part_t a, part_t b) -> std::optional<part_t> {
    const auto begin = std::max(a.begin, b.begin);
    const auto end = std::min(a.end, b.end);

    if (begin != end) {
        return part_t {begin, end};
    }
    return std::nullopt;
}

auto difference_touching_one_side(part_t full_part, part_t b) -> part_t {
    if (full_part.begin == b.begin) {
        return part_t {b.end, full_part.end};
    }

    if (full_part.end != b.end) [[unlikely]] {
        throw_exception("part needs to be touching one side");
    }

    return part_t {full_part.begin, b.begin};
}

auto difference_not_touching(part_t full_part, part_t b) -> std::pair<part_t, part_t> {
    return {
        part_t {full_part.begin, b.begin},
        part_t {b.end, full_part.end},
    };
}

//
// Parts List
//

auto is_part_inside_line(part_t part, ordered_line_t line) -> bool {
    if (is_horizontal(line)) {
        const auto x_end = to_grid(part.end, line.p0.x);
        return x_end <= line.p1.x;
    }

    const auto y_end = to_grid(part.end, line.p0.y);
    return y_end <= line.p1.y;
}

auto sort_and_validate_segment_parts(std::span<part_t> parts, ordered_line_t line)
    -> void {
    // part inside line
    for (const auto part : parts) {
        if (!is_part_inside_line(part, line)) [[unlikely]] {
            throw_exception("part is not part of line");
        }
    }

    // parts overlapping or touching?
    std::ranges::sort(parts);
    const auto part_overlapping
        = [](part_t part0, part_t part1) -> bool { return part0.end >= part1.begin; };
    const auto any_overlapping
        = std::ranges::adjacent_find(parts, part_overlapping) != parts.end();

    if (any_overlapping) {
        throw_exception("some parts are overlapping");
    }
}

auto validate_segment_parts(std::span<const part_t> parts, ordered_line_t line) -> void {
    using parts_vector_t = typename folly::small_vector<part_t, 4>;

    auto copy = parts_vector_t {parts.begin(), parts.end()};
    sort_and_validate_segment_parts(copy, line);
}

auto a_inside_b(part_t a, part_t b) -> bool {
    return b.begin <= a.begin && a.end <= b.end;
}

auto a_inside_b_not_touching(part_t a, part_t b) -> bool {
    return b.begin < a.begin && a.end < b.end;
}

auto a_disjoint_to_b(part_t a, part_t b) -> bool {
    return a.begin >= b.end || a.end <= b.begin;
}

auto a_inside_b_touching_one_side(part_t a, part_t b) -> bool {
    return a_inside_b(a, b) && (b.begin == a.begin ^ b.end == a.end);
}

auto a_equal_b(part_t a, part_t b) -> bool {
    return a == b;
}

auto a_overlapps_b(part_t a, part_t b) -> bool {
    return (a.end > b.begin && a.end <= b.end)  //
           || (b.end > a.begin && b.end <= a.end);
}

auto format(InclusionResult state) -> std::string {
    switch (state) {
        using enum InclusionResult;

        case fully_included:
            return "fully_included";
        case not_included:
            return "not_included";
        case partially_overlapping:
            return "partially_overlapping";
    }
    throw_exception("Don't know how to convert InclusionState to string.");
}

auto is_part_included(std::span<const part_t> parts, part_t query) -> InclusionResult {
    for (const auto part : parts) {
        // parts can not touch or overlapp, so we can return early
        if (a_inside_b(query, part)) {
            return InclusionResult::fully_included;
        }
        if (a_overlapps_b(query, part)) {
            return InclusionResult::partially_overlapping;
        }
    }

    return InclusionResult::not_included;
}

}  // namespace logicsim