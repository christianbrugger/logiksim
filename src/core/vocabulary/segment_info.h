#ifndef LOGICSIM_VOCABULARY_SEGMENT_INFO_H
#define LOGICSIM_VOCABULARY_SEGMENT_INFO_H

#include "core/format/struct.h"
#include "core/vocabulary/ordered_line.h"
#include "core/vocabulary/segment_point_type.h"

#include <string>
#include <type_traits>

namespace logicsim {

struct segment_info_t {
    ordered_line_t line {};

    SegmentPointType p0_type {SegmentPointType::shadow_point};
    SegmentPointType p1_type {SegmentPointType::shadow_point};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const segment_info_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const segment_info_t &other) const = default;
};

static_assert(sizeof(segment_info_t) == 10);
static_assert(std::is_aggregate_v<segment_info_t>);

}  // namespace logicsim

#endif
