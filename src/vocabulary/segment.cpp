#include "vocabulary/segment.h"

#include <fmt/core.h>

namespace logicsim {

auto segment_t::format() const -> std::string {
    if (!static_cast<bool>(*this)) {
        return fmt::format("<NullSegment>", segment_index, wire_id);
    }
    return fmt::format("<Wire {}, Segment {}>", wire_id, segment_index);
}

}  // namespace logicsim
