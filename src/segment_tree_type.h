#ifndef LOGIKSIM_SEGMENT_TREE_TYPE_H
#define LOGIKSIM_SEGMENT_TREE_TYPE_H

#include "format/struct.h"
#include "vocabulary.h"

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

struct segment_info_t {
    ordered_line_t line {};

    SegmentPointType p0_type {SegmentPointType::shadow_point};
    SegmentPointType p1_type {SegmentPointType::shadow_point};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const segment_info_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const segment_info_t &other) const = default;
};

static_assert(sizeof(segment_info_t) == 10);

}  // namespace logicsim

#endif
