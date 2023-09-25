#include "vocabulary/segment_part.h"

#include <fmt/core.h>

namespace logicsim {

auto segment_part_t::format() const -> std::string {
    return fmt::format("<Element {}, Segment {}, part {}-{}>", segment.element_id,
                       segment.segment_index, part.begin, part.end);
}

}  // namespace logicsim
