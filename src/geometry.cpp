#include "geometry.h"

#include "algorithm/round.h"
#include "exception.h"
#include "geometry.h"
#include "vocabulary.h"

#include <numbers>

namespace logicsim {

//
// orientation_t
//

auto is_horizontal(orientation_t orientation) noexcept -> bool {
    using enum orientation_t;
    return orientation == left || orientation == right;
}

auto is_vertical(orientation_t orientation) noexcept -> bool {
    using enum orientation_t;
    return orientation == up || orientation == down;
}

//
// grid_t
//

namespace {
auto clamp_discrete_to_grid(grid_fine_t grid_fine) -> grid_t {
    const auto clamped = clamp_to_grid(grid_fine);
    return grid_t {gsl::narrow_cast<grid_t::value_type>(double {clamped})};
}
}  // namespace

auto to_rounded(grid_fine_t v) -> grid_t {
    return clamp_discrete_to_grid(round(v));
}

auto to_floored(grid_fine_t v) -> grid_t {
    return clamp_discrete_to_grid(floor(v));
}

auto to_ceiled(grid_fine_t v) -> grid_t {
    return clamp_discrete_to_grid(ceil(v));
}

//
// grid_fine_t
//

auto clamp_to_grid(grid_fine_t grid_fine) -> grid_fine_t {
    return std::clamp(grid_fine,                    //
                      grid_fine_t {grid_t::min()},  //
                      grid_fine_t {grid_t::max()});
}

auto round(grid_fine_t v) -> grid_fine_t {
    return grid_fine_t {round_fast(double {v})};
}

auto floor(grid_fine_t v) -> grid_fine_t {
    return grid_fine_t {std::floor(double {v})};
}

auto ceil(grid_fine_t v) -> grid_fine_t {
    return grid_fine_t {std::ceil(double {v})};
}

//
// line_t
//
auto is_horizontal(point_t p0, point_t p1) noexcept -> bool {
    return p0.y == p1.y;
}

auto is_vertical(point_t p0, point_t p1) noexcept -> bool {
    return p0.x == p1.x;
}

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

auto distance(part_t part) -> int {
    return part.end.value - part.begin.value;
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

auto to_orientation_p0(ordered_line_t line) -> orientation_t {
    return to_orientation(line.p1, line.p0);
}

auto to_orientation_p1(ordered_line_t line) -> orientation_t {
    return to_orientation(line.p0, line.p1);
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
            throw_exception("undirected orientation has no angle");
    };
    throw_exception("unhandeled orientation");
}

auto is_endpoint(point_t point, line_t line) -> bool {
    return line.p0 == point || line.p1 == point;
}

auto is_endpoint(point_t point, ordered_line_t line) -> bool {
    return is_endpoint(point, line_t {line});
}

auto add_unchecked(grid_t grid, int delta) -> grid_t {
    return grid_t {gsl::narrow_cast<grid_t::value_type>(grid.value + delta)};
}

auto add_unchecked(point_t point, int dx, int dy) -> point_t {
    return point_t {
        add_unchecked(point.x, dx),
        add_unchecked(point.y, dy),
    };
}

auto add_unchecked(line_t line, int dx, int dy) -> line_t {
    return line_t {
        add_unchecked(line.p0, dx, dy),
        add_unchecked(line.p1, dx, dy),
    };
}

auto add_unchecked(ordered_line_t line, int dx, int dy) -> ordered_line_t {
    return ordered_line_t {
        add_unchecked(line.p0, dx, dy),
        add_unchecked(line.p1, dx, dy),
    };
}

//
// rect_t
//

auto enclosing_rect(rect_fine_t rect) -> rect_t {
    return rect_t {
        point_t {
            to_floored(rect.p0.x),
            to_floored(rect.p0.y),
        },
        point_t {
            to_ceiled(rect.p1.x),
            to_ceiled(rect.p1.y),
        },
    };
}

auto enclosing_rect(rect_t a, rect_t b) -> rect_t {
    return rect_t {
        point_t {std::min(a.p0.x, b.p0.x), std::min(a.p0.y, b.p0.y)},
        point_t {std::max(a.p1.x, b.p1.x), std::max(a.p1.y, b.p1.y)},
    };
}

auto enclosing_rect(rect_t rect, ordered_line_t line) -> rect_t {
    return rect_t {
        point_t {std::min(rect.p0.x, line.p0.x), std::min(rect.p0.y, line.p0.y)},
        point_t {std::max(rect.p1.x, line.p1.x), std::max(rect.p1.y, line.p1.y)},
    };
}

auto to_rect(point_fine_t center, grid_fine_t size) -> rect_fine_t {
    const auto half = size / 2.0;

    return rect_fine_t {
        point_fine_t {center.x - half, center.y - half},
        point_fine_t {center.x + half, center.y + half},
    };
}

auto get_center(rect_fine_t rect) -> point_fine_t {
    return point_fine_t {
        (rect.p0.x + rect.p1.x) / 2.0,
        (rect.p0.y + rect.p1.y) / 2.0,
    };
}

//
// offset_t
//

auto to_offset(grid_t x, grid_t reference) -> offset_t {
    const auto value = x.value - reference.value;

    static_assert(sizeof(value) > sizeof(grid_t::value_type));
    static_assert(std::is_signed_v<decltype(value)>);

    return offset_t {gsl::narrow_cast<offset_t::value_type>(value)};
}

auto to_grid(offset_t offset, grid_t reference) -> grid_t {
    const auto value = reference.value + offset.value;

    static_assert(sizeof(value) > sizeof(grid_t::value_type));
    static_assert(std::is_signed_v<decltype(value)>);

    return grid_t {gsl::narrow<grid_t::value_type>(value)};
}

//
// Interpolation
//

auto interpolate_1d(grid_t v0, grid_t v1, double ratio) -> grid_fine_t {
    return grid_fine_t {v0.value + (v1.value - v0.value) * ratio};
}

auto interpolate_line_1d(point_t p0, point_t p1, time_t t0, time_t t1, time_t t_select)
    -> point_fine_t {
    assert(t0 < t1);

    if (t_select <= t0) {
        return point_fine_t {p0};
    }
    if (t_select >= t1) {
        return point_fine_t {p1};
    }

    const double alpha = static_cast<double>((t_select.value - t0.value).count()) /
                         static_cast<double>((t1.value - t0.value).count());

    if (is_horizontal(line_t {p0, p1})) {
        return point_fine_t {interpolate_1d(p0.x, p1.x, alpha), grid_fine_t(p0.y)};
    }
    return point_fine_t {grid_fine_t {p0.x}, interpolate_1d(p0.y, p1.y, alpha)};
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

        const auto xmin = to_floored(rect.p0.x);
        const auto xmax = to_ceiled(rect.p1.x);

        const auto begin = std::clamp(line.p0.x, xmin, xmax);
        const auto end = std::clamp(line.p1.x, xmin, xmax);

        return std::make_tuple(line.p0.x, begin, end);
    }

    // vertical
    const auto ymin = to_floored(rect.p0.y);
    const auto ymax = to_ceiled(rect.p1.y);

    const auto begin = std::clamp(line.p0.y, ymin, ymax);
    const auto end = std::clamp(line.p1.y, ymin, ymax);

    return std::make_tuple(line.p0.y, begin, end);
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

auto to_point(ordered_line_t full_line, offset_t offset) -> point_t {
    if (is_horizontal(full_line)) {
        const auto x = to_grid(offset, full_line.p0.x);
        if (x > full_line.p1.x) [[unlikely]] {
            throw_exception("offset is not within line");
        }
        return point_t {x, full_line.p0.y};
    }

    const auto y = to_grid(offset, full_line.p0.y);
    if (y > full_line.p1.y) [[unlikely]] {
        throw_exception("offset is not within line");
    }
    return point_t {full_line.p0.x, y};
}

auto to_offset(ordered_line_t full_line, point_t point) -> offset_t {
    const auto line = ordered_line_t {full_line.p0, point};

    if (line.p1 > full_line.p1) [[unlikely]] {
        throw_exception("point is not part of full_line");
    }

    return to_part(line).end;
}

auto to_line(ordered_line_t full_line, part_t part) -> ordered_line_t {
    if (!is_part_valid(part, full_line)) {
        throw_exception("part needs to be within line");
    }

    const auto x = full_line.p0.x;
    const auto y = full_line.p0.y;

    if (is_horizontal(full_line)) {
        // horizontal
        return ordered_line_t {point_t {to_grid(part.begin, x), y},
                               point_t {to_grid(part.end, x), y}};
    }

    // vertical
    return ordered_line_t {point_t {x, to_grid(part.begin, y)},
                           point_t {x, to_grid(part.end, y)}};
}

auto is_part_valid(part_t part, ordered_line_t full_line) -> bool {
    return part.end <= to_part(full_line).end;
}

auto to_segment_part(segment_t segment, ordered_line_t line) -> segment_part_t {
    return segment_part_t {segment, to_part(line)};
}

auto intersect(part_t a, part_t b) -> std::optional<part_t> {
    const auto begin = std::max(a.begin, b.begin);
    const auto end = std::min(a.end, b.end);

    if (end > begin) {
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

auto sort_and_validate_segment_parts(std::span<part_t> parts, ordered_line_t line)
    -> void {
    // part inside line
    for (const auto part : parts) {
        if (!is_part_valid(part, line)) [[unlikely]] {
            throw_exception("part is not part of line");
        }
    }

    // parts overlapping or touching?
    std::ranges::sort(parts);
    const auto part_overlapping = [](part_t part0, part_t part1) -> bool {
        return part0.end >= part1.begin;
    };
    if (std::ranges::adjacent_find(parts, part_overlapping) != parts.end()) {
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

auto a_disjoint_b(part_t a, part_t b) -> bool {
    return a.begin >= b.end || a.end <= b.begin;
}

auto a_inside_b_touching_one_side(part_t a, part_t b) -> bool {
    return a_inside_b(a, b) && ((b.begin == a.begin) ^ (b.end == a.end));
}

auto a_inside_b_touching_begin(part_t a, part_t b) -> bool {
    return a.begin == b.begin && a.end < b.end;
}

auto a_inside_b_touching_end(part_t a, part_t b) -> bool {
    return a.begin > b.begin && a.end == b.end;
}

auto a_equal_b(part_t a, part_t b) -> bool {
    return a == b;
}

auto a_overlapps_any_of_b(part_t a, part_t b) -> bool {
    return (a.end > b.begin && a.end <= b.end)  //
           || (b.end > a.begin && b.end <= a.end);
}

auto a_overlapps_b_begin(part_t a, part_t b) -> bool {
    return a.begin <= b.begin && a.end > b.begin && a.end < b.end;
}

auto a_overlapps_b_end(part_t a, part_t b) -> bool {
    return a.begin > b.begin && a.begin < b.end && a.end >= b.end;
}

}  // namespace logicsim