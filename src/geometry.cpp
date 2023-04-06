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

// order points within lines
auto order_points(line_t line) noexcept -> line_t {
    auto [p0, p1] = sorted(line.p0, line.p1);
    return line_t {p0, p1};
}

// order lines and points within lines
auto order_points(const line_t line0, const line_t line1) noexcept
    -> std::tuple<line_t, line_t> {
    auto a = order_points(line0);
    auto b = order_points(line1);

    if (a.p0 <= b.p0) {
        return std::make_tuple(a, b);
    }
    return std::make_tuple(b, a);
}

// fast distance for horitonal or vertical lines
auto distance(line_t line) -> int {
    auto dx = line.p1.x.value - line.p0.x.value;
    auto dy = line.p1.y.value - line.p0.y.value;

    // ensure enough precision, through promotion
    static_assert(sizeof(dx) > sizeof(line.p0.x.value));
    return std::abs((dx == 0) ? dy : dx);
}

auto to_orientation(point_t p0, point_t p1) -> orientation_t {
    return to_orientation(line_t {p0, p1});
}

auto to_orientation(line_t line) -> orientation_t {
    using enum orientation_t;

    if (line.p1.x > line.p0.x) {
        return orientation_t::right;
    }
    if (line.p1.x < line.p0.x) {
        return orientation_t::left;
    }
    if (line.p1.y < line.p0.y) {
        return orientation_t::up;
    }
    if (line.p1.y > line.p0.y) {
        return orientation_t::down;
    }

    throw_exception("unreachable code");
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

//
// Segments
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

auto to_part(line_t line) -> part_t {
    const auto ordered_line = order_points(line);

    if (is_horizontal(line)) {
        const auto end = to_offset(ordered_line.p1.x, ordered_line.p0.x);
        return part_t {offset_t {0}, end};
    }
    const auto end = to_offset(ordered_line.p1.y, ordered_line.p0.y);
    return part_t {offset_t {0}, end};
}

auto get_segment_reference_begin_end(line_t line, rect_fine_t rect) {
    const auto ordered_line = order_points(line);

    if (is_horizontal(line)) {
        // horizontal
        const auto xmin = clamp_to<grid_t::value_type>(std::floor(rect.p0.x));
        const auto xmax = clamp_to<grid_t::value_type>(std::ceil(rect.p1.x));

        const auto begin = std::clamp(ordered_line.p0.x.value, xmin, xmax);
        const auto end = std::clamp(ordered_line.p1.x.value, xmin, xmax);

        return std::make_tuple(ordered_line.p0.x.value, begin, end);
    }

    // vertical
    const auto ymin = clamp_to<grid_t::value_type>(std::floor(rect.p0.y));
    const auto ymax = clamp_to<grid_t::value_type>(std::ceil(rect.p1.y));

    const auto begin = std::clamp(ordered_line.p0.y.value, ymin, ymax);
    const auto end = std::clamp(ordered_line.p1.y.value, ymin, ymax);

    return std::make_tuple(ordered_line.p0.y.value, begin, end);
}

auto to_part(line_t line, rect_fine_t rect) -> std::optional<part_t> {
    const auto [reference, begin, end] = get_segment_reference_begin_end(line, rect);

    if (begin == end) {
        return std::nullopt;
    }
    return part_t {to_offset(begin, reference), to_offset(end, reference)};
}

auto to_line(line_t line, part_t part) -> line_t {
    const auto p_reference = std::min(line.p0, line.p1);

    const auto x = p_reference.x;
    const auto y = p_reference.y;

    if (is_horizontal(line)) {
        // horizontal
        return line_t {point_t {to_grid(part.begin, x), y},
                       point_t {to_grid(part.end, x), y}};
    }

    // vertical
    return line_t {point_t {x, to_grid(part.begin, y)},
                   point_t {x, to_grid(part.end, y)}};
}

//
// Parts List
//

auto is_part_inside_line(part_t part, line_t line) -> bool {
    const auto sorted_line = order_points(line);

    if (is_horizontal(sorted_line)) {
        const auto x_end = to_grid(part.end, sorted_line.p0.x);
        return x_end <= sorted_line.p1.x;
    }

    const auto y_end = to_grid(part.end, sorted_line.p0.y);
    return y_end <= sorted_line.p1.y;
}

auto sort_and_validate_segment_parts(std::span<part_t> parts, line_t line) -> void {
    // part inside line
    for (const auto part : parts) {
        if (!is_part_inside_line(part, line)) [[unlikely]] {
            print(part, line);
            throw_exception("part is not part of line");
        }
    }

    // overlapping or touching?
    std::ranges::sort(parts);
    const auto part_overlapping
        = [](part_t part0, part_t part1) -> bool { return part0.end >= part1.begin; };
    if (std::ranges::adjacent_find(parts, part_overlapping) != parts.end()) {
        throw_exception("some parts are overlapping");
    }
}

auto validate_segment_parts(std::span<const part_t> parts, line_t line) -> void {
    using parts_vector_t = folly::small_vector<part_t, 4>;

    auto copy = parts_vector_t {parts.begin(), parts.end()};
    sort_and_validate_segment_parts(copy, line);
}

}  // namespace logicsim