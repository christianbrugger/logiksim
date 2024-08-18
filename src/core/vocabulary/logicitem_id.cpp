#include "vocabulary/logicitem_id.h"

#include <fmt/core.h>

namespace logicsim {

auto logicitem_id_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
