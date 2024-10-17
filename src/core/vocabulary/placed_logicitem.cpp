#include "core/vocabulary/placed_logicitem.h"

#include <fmt/core.h>

namespace logicsim {

auto PlacedLogicItem::format() const -> std::string {
    return fmt::format("<{} {}>", position, definition);
}

}  // namespace logicsim
