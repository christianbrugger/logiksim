#include "core/vocabulary/segment_index.h"

#include <fmt/core.h>

namespace logicsim {

auto segment_index_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
