#ifndef LOGICSIM_GEOMETRY_SEGMENT_INFO_H
#define LOGICSIM_GEOMETRY_SEGMENT_INFO_H

#include <tuple>

namespace logicsim {

struct part_t;
struct segment_info_t;

[[nodiscard]] auto order_points(segment_info_t segment0, segment_info_t segment1)
    -> std::tuple<segment_info_t, segment_info_t>;
[[nodiscard]] auto adjust(segment_info_t segment, part_t part) -> segment_info_t;
[[nodiscard]] auto merge_touching(const segment_info_t segment_info_0,
                                  const segment_info_t segment_info_1) -> segment_info_t;

}  // namespace logicsim

#endif
