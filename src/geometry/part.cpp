#include "geometry/part.h"

#include "geometry/grid.h"
#include "geometry/offset.h"
#include "geometry/orientation.h"
#include "vocabulary/ordered_line.h"
#include "vocabulary/part.h"
#include "vocabulary/rect_fine.h"
#include "vocabulary/segment.h"
#include "vocabulary/segment_part.h"

#include <stdexcept>

namespace logicsim {

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

//
// intersect
//

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
        throw std::domain_error("part needs to be touching one side");
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
// to part
//

auto to_part(ordered_line_t line) -> part_t {
    return part_t {offset_t {0}, to_offset(line)};
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
        throw std::domain_error("sub_line needs to be within line");
    }

    return part_t {begin, end};
}

auto to_line(ordered_line_t full_line, part_t part) -> ordered_line_t {
    if (!is_part_valid(part, full_line)) {
        throw std::domain_error("part needs to be within line");
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

//
// valid
//

auto distance(part_t part) -> int {
    return part.end.value - part.begin.value;
}

auto is_part_valid(part_t part, ordered_line_t full_line) -> bool {
    return part.end <= to_part(full_line).end;
}

auto to_segment_part(segment_t segment, ordered_line_t line) -> segment_part_t {
    return segment_part_t {segment, to_part(line)};
}

}  // namespace logicsim
