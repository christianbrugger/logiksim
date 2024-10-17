#include "core/vocabulary/segment_point_type.h"

namespace logicsim {

template <>
auto format(SegmentPointType type) -> std::string {
    switch (type) {
        using enum SegmentPointType;

        case input:
            return "input";
        case output:
            return "output";
        case corner_point:
            return "corner_point";
        case cross_point:
            return "cross_point";

        case shadow_point:
            return "shadow_point";
        case new_unknown:
            return "new_unknown";
    }
    std::terminate();
}

auto is_corner_point(SegmentPointType point_type) -> bool {
    return point_type == SegmentPointType::corner_point;
}

auto is_cross_point(SegmentPointType point_type) -> bool {
    return point_type == SegmentPointType::cross_point;
}

auto is_connection(SegmentPointType point_type) -> bool {
    using enum SegmentPointType;
    return point_type == input || point_type == output;
}

}  // namespace logicsim
