#include "core/vocabulary/circuit_id.h"

#include <fmt/format.h>

namespace logicsim {

auto circuit_id_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
