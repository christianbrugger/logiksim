#include "core/vocabulary/internal_connection.h"

#include <fmt/format.h>

namespace logicsim {

auto internal_connection_t::format() -> std::string {
    return fmt::format("o{}-i{}", output, input);
}

}  // namespace logicsim
