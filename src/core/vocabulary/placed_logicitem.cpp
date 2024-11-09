#include "core/vocabulary/placed_logicitem.h"

#include "core/allocated_size/trait.h"

#include <fmt/core.h>

namespace logicsim {

auto PlacedLogicItem::format() const -> std::string {
    return fmt::format("<{} {}>", position, definition);
}

auto PlacedLogicItem::allocated_size() const -> std::size_t {
    return get_allocated_size(definition);
}

}  // namespace logicsim
