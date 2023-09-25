#include "vocabulary/connection.h"

#include <fmt/core.h>

namespace logicsim {

auto connection_t::format() const -> std::string {
    return fmt::format("<Element {}, Conection {}>", element_id, connection_id);
}

}  // namespace logicsim
