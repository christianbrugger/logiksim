#ifndef LOGICSIM_CORE_VOCABULARY_ENDPOINTS_H
#define LOGICSIM_CORE_VOCABULARY_ENDPOINTS_H

#include "core/format/struct.h"
#include "core/vocabulary/segment_point_type.h"

#include <string>
#include <type_traits>

namespace logicsim {

struct endpoints_t {
    SegmentPointType p0_type {SegmentPointType::shadow_point};
    SegmentPointType p1_type {SegmentPointType::shadow_point};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const endpoints_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const endpoints_t &other) const = default;
};

static_assert(sizeof(endpoints_t) == 2);
static_assert(std::is_aggregate_v<endpoints_t>);

}  // namespace logicsim

#endif
