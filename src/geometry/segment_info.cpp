#include "geometry/segment_info.h"

#include "geometry/part.h"
#include "vocabulary/part.h"
#include "vocabulary/point.h"
#include "vocabulary/segment_info.h"

#include <stdexcept>

namespace logicsim {

auto order_points(segment_info_t a, segment_info_t b)
    -> std::tuple<segment_info_t, segment_info_t> {
    if (a.line <= b.line) {
        return std::make_tuple(a, b);
    }
    return std::make_tuple(b, a);
}

auto adjust(const segment_info_t segment_info, const part_t part) -> segment_info_t {
    const auto new_line = to_line(segment_info.line, part);

    const auto p0_changed = new_line.p0 != segment_info.line.p0;
    const auto p1_changed = new_line.p1 != segment_info.line.p1;

    return segment_info_t {
        .line = new_line,

        .p0_type = p0_changed ? SegmentPointType::shadow_point : segment_info.p0_type,
        .p1_type = p1_changed ? SegmentPointType::shadow_point : segment_info.p1_type,
    };
}

auto merge_touching(const segment_info_t segment_info_0,
                    const segment_info_t segment_info_1) -> segment_info_t {
    const auto [a, b] = order_points(segment_info_0, segment_info_1);

    if (a.line.p1 != b.line.p0) [[unlikely]] {
        throw std::runtime_error("segments need to have common shared point");
    }

    return segment_info_t {
        .line = ordered_line_t {a.line.p0, b.line.p1},

        .p0_type = a.p0_type,
        .p1_type = b.p1_type,
    };
}

auto to_point_and_type(const segment_info_t &segment_info)
    -> std::array<std::pair<point_t, SegmentPointType>, 2> {
    return {
        std::pair {segment_info.line.p0, segment_info.p0_type},
        std::pair {segment_info.line.p1, segment_info.p1_type},
    };
}

}  // namespace logicsim
