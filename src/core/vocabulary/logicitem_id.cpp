#include "core/vocabulary/logicitem_id.h"

#include <fmt/format.h>

namespace logicsim {

auto logicitem_id_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
