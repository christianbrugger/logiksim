#include "vocabulary/connection.h"

#include <fmt/core.h>

namespace logicsim {

auto connection_t::format() const -> std::string {
    if (*this) {
        return fmt::format("Element_{}-{}", element_id, connection_id);
    }
    return "---";
}

}  // namespace logicsim
