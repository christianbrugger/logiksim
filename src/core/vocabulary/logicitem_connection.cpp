#include "vocabulary/logicitem_connection.h"

#include <fmt/core.h>

namespace logicsim {

auto logicitem_connection_t::format() const -> std::string {
    if (*this) {
        return fmt::format("LogicItem_{}-{}-{}", logicitem_id, connection_id,
                           orientation);
    }
    return "---";
}

}  // namespace logicsim
