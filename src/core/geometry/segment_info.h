#ifndef LOGICSIM_GEOMETRY_SEGMENT_INFO_H
#define LOGICSIM_GEOMETRY_SEGMENT_INFO_H

#include "core/vocabulary/orientation.h"
#include "core/vocabulary/segment_point_type.h"

#include <array>
#include <tuple>
#include <utility>  // pair

namespace logicsim {

struct part_t;
struct endpoints_t;
struct segment_info_t;
struct point_t;
struct ordered_line_t;

[[nodiscard]] auto order_points(segment_info_t a, segment_info_t b)
    -> std::tuple<segment_info_t, segment_info_t>;

[[nodiscard]] auto to_point_type_orientation(const segment_info_t &segment_info)
    -> std::array<std::tuple<point_t, SegmentPointType, orientation_t>, 2>;

[[nodiscard]] auto adjust(segment_info_t segment, part_t part) -> segment_info_t;

[[nodiscard]] auto merge_touching(segment_info_t segment_info_0,
                                  segment_info_t segment_info_1) -> segment_info_t;

[[nodiscard]] auto to_point_type(const segment_info_t &segment_info)
    -> std::array<std::pair<point_t, SegmentPointType>, 2>;

auto set_segment_point_type(segment_info_t &info, point_t position,
                            SegmentPointType type) -> void;

[[nodiscard]] auto updated_segment_info(segment_info_t segment_info, point_t position,
                                        SegmentPointType type) -> segment_info_t;

[[nodiscard]] auto get_segment_point_type(const segment_info_t &info,
                                          point_t position) -> SegmentPointType;

[[nodiscard]] auto get_endpoints(const segment_info_t &info) -> endpoints_t;
[[nodiscard]] auto to_segment_info(ordered_line_t line,
                                   endpoints_t endpoints) -> segment_info_t;

}  // namespace logicsim

#endif
