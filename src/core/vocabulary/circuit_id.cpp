#include "core/vocabulary/circuit_id.h"

#include <fmt/core.h>

namespace logicsim {

auto circuit_id_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
