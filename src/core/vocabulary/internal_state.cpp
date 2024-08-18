#include "vocabulary/internal_state.h"

#include <fmt/core.h>

namespace logicsim {

auto internal_state_t::format() const -> std::string {
    return fmt::format("State_{}_{}", element_id, internal_state_index);
}

}  // namespace logicsim
