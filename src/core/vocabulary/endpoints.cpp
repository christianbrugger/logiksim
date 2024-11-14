#include "core/vocabulary/endpoints.h"

#include <fmt/core.h>

namespace logicsim {

auto endpoints_t::format() const -> std::string {
    return fmt::format("(p0_type = {}, p1_type = {})", p0_type, p1_type);
}

}  // namespace logicsim
