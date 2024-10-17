#include "core/vocabulary/connection_id.h"

#include <fmt/core.h>

namespace logicsim {

auto connection_id_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
