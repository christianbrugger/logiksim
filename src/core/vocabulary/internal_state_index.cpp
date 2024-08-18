#include "vocabulary/internal_state_index.h"

#include <fmt/core.h>

namespace logicsim {

auto internal_state_index_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
