#ifndef LOGICSIM_VOCABULARY_SEGMENT_POINT_TYPE_H
#define LOGICSIM_VOCABULARY_SEGMENT_POINT_TYPE_H

#include "format/enum.h"

#include <cstdint>
#include <string>

namespace logicsim {

enum class SegmentPointType : uint8_t {
    // has collision
    input,
    output,
    corner_point,
    cross_point,

    // no collision
    shadow_point,
    // unknown state
    new_unknown,
};

template <>
auto format(SegmentPointType type) -> std::string;

auto is_cross_point(SegmentPointType point_type) -> bool;
auto is_connection(SegmentPointType point_type) -> bool;

}  // namespace logicsim

#endif
