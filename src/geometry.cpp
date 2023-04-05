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

auto get_segment_part(line_t line) -> part_t {
    const auto ordered_line = order_points(line);

    if (is_horizontal(line)) {
        return part_t {ordered_line.p0.x, ordered_line.p1.x};
    }
    return part_t {ordered_line.p0.y, ordered_line.p1.y};
}

auto get_segment_begin_end(line_t line, rect_fine_t rect) {
    const auto ordered_line = order_points(line);

    if (is_horizontal(line)) {
        const auto xmin = clamp_to<grid_t::value_type>(std::floor(rect.p0.x));
        const auto xmax = clamp_to<grid_t::value_type>(std::ceil(rect.p1.x));

        const auto begin = std::clamp(ordered_line.p0.x.value, xmin, xmax);
        const auto end = std::clamp(ordered_line.p1.x.value, xmin, xmax);

        return std::make_pair(begin, end);
    }

    // vertical
    const auto ymin = clamp_to<grid_t::value_type>(std::floor(rect.p0.y));
    const auto ymax = clamp_to<grid_t::value_type>(std::ceil(rect.p1.y));

    const auto begin = std::clamp(ordered_line.p0.y.value, ymin, ymax);
    const auto end = std::clamp(ordered_line.p1.y.value, ymin, ymax);

    return std::make_pair(begin, end);
}

auto get_segment_part(line_t line, rect_fine_t rect) -> std::optional<part_t> {
    const auto [begin, end] = get_segment_begin_end(line, rect);

    if (begin == end) {
        return std::nullopt;
    }
    return part_t {begin, end};
}

auto get_selected_segment(line_t segment, part_t selection) -> line_t {
    if (is_horizontal(segment)) {
        const auto y = segment.p0.y;
        return line_t {point_t {selection.begin, y}, point_t {selection.end, y}};
    }

    // vertical
    const auto x = segment.p0.x;
    return line_t {point_t {x, selection.begin}, point_t {x, selection.end}};
}

}  // namespace logicsim