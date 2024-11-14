#include "core/geometry/segment_info.h"

#include "core/geometry/part.h"
#include "core/vocabulary/endpoints.h"
#include "core/vocabulary/part.h"
#include "core/vocabulary/point.h"
#include "core/vocabulary/segment_info.h"

#include <stdexcept>

namespace logicsim {

auto order_points(segment_info_t a,
                  segment_info_t b) -> std::tuple<segment_info_t, segment_info_t> {
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

auto set_segment_point_type(segment_info_t &info, point_t position,
                            SegmentPointType type) -> void {
    if (info.line.p0 == position) {
        info.p0_type = type;
    } else if (info.line.p1 == position) {
        info.p1_type = type;
    } else {
        throw std::runtime_error("Position needs to be an endpoint of the segment.");
    }
}

auto updated_segment_info(segment_info_t segment_info, point_t position,
                          SegmentPointType type) -> segment_info_t {
    set_segment_point_type(segment_info, position, type);
    return segment_info;
}

auto get_segment_point_type(const segment_info_t &info,
                            point_t position) -> SegmentPointType {
    if (info.line.p0 == position) {
        return info.p0_type;
    }
    if (info.line.p1 == position) {
        return info.p1_type;
    };
    throw std::runtime_error("Position needs to be an endpoint of the segment.");
}

auto get_endpoints(const segment_info_t &info) -> endpoints_t {
    return endpoints_t {
        .p0_type = info.p0_type,
        .p1_type = info.p1_type,
    };
}

auto to_segment_info(ordered_line_t line, endpoints_t endpoints) -> segment_info_t {
    return segment_info_t {
        .line = line,
        .p0_type = endpoints.p0_type,
        .p1_type = endpoints.p1_type,
    };
}

}  // namespace logicsim
