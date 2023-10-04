#ifndef LOGICSIM_GEOMETRY_PART_H
#define LOGICSIM_GEOMETRY_PART_H

#include <optional>
#include <utility>

namespace logicsim {

struct ordered_line_t;
struct rect_fine_t;
struct part_t;
struct segment_t;
struct segment_part_t;

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

[[nodiscard]] auto intersect(part_t a, part_t b) -> std::optional<part_t>;
[[nodiscard]] auto difference_touching_one_side(part_t full_part, part_t b) -> part_t;
[[nodiscard]] auto difference_not_touching(part_t full_part, part_t b)
    -> std::pair<part_t, part_t>;

[[nodiscard]] auto to_part(ordered_line_t full_line) -> part_t;
[[nodiscard]] auto to_part(ordered_line_t full_line, rect_fine_t rect)
    -> std::optional<part_t>;
[[nodiscard]] auto to_part(ordered_line_t full_line, ordered_line_t line) -> part_t;
[[nodiscard]] auto to_line(ordered_line_t full_line, part_t part) -> ordered_line_t;

[[nodiscard]] auto distance(part_t part) -> int;
[[nodiscard]] auto is_part_valid(part_t part, ordered_line_t full_line) -> bool;
[[nodiscard]] auto to_segment_part(segment_t segment, ordered_line_t sub_line)
    -> segment_part_t;

}  // namespace logicsim

#endif
