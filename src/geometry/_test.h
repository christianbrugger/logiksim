#ifndef LOGICSIM_GEOMETRY__TEST
#define LOGICSIM_GEOMETRY__TEST

#include "vocabulary.h"

namespace logicsim {

//
// rect
//

//
// interpolation
//

[[nodiscard]] auto interpolate_1d(grid_t v0, grid_t v1, double ratio) -> grid_fine_t;
[[nodiscard]] auto interpolate_line_1d(point_t p0, point_t p1, time_t t0, time_t t1,
                                       time_t t_select) -> point_fine_t;

//
// part_t
//

//
// offset
//

}  // namespace logicsim

///
///
///
///
///
///
///

namespace logicsim {

//
// Parts List
//

auto sort_and_validate_segment_parts(std::span<part_t> parts, ordered_line_t line)
    -> void;
auto validate_segment_parts(std::span<const part_t> parts, ordered_line_t line) -> void;
template <typename Container = std::vector<part_t>>
auto sort_and_merge_parts(Container &entries) -> void;
template <typename Container = std::vector<part_t>>
auto add_part(Container &entries, part_t new_part) -> void;
template <typename Container = std::vector<part_t>>
auto remove_part(Container &entries, part_t removing) -> void;

//
// Part List Copying
//

template <typename Container = std::vector<part_t>>
[[nodiscard]] auto copy_parts(const Container &source_entries, part_t part_destination)
    -> Container;
template <typename Container = std::vector<part_t>>
auto copy_parts(const Container &source_entries, Container &destination_entries,
                part_t part_destination) -> void;
template <typename Container = std::vector<part_t>>
[[nodiscard]] auto copy_parts(const Container &source_entries,
                              part_copy_definition_t parts) -> Container;
template <typename Container = std::vector<part_t>>
auto copy_parts(const Container &source_entries, Container &destination_entries,
                part_copy_definition_t parts) -> void;
template <typename Container = std::vector<part_t>>
auto move_parts(Container &source_entries, Container &destination_entries,
                part_copy_definition_t parts) -> void;

}  // namespace logicsim

#endif
