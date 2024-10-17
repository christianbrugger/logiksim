#include "core/vocabulary/drawable_element.h"

#include <fmt/core.h>

namespace logicsim {

auto DrawableLogicItem::format() const -> std::string {
    return fmt::format("{}-{}", logicitem_id, state);
}

auto DrawableDecoration::format() const -> std::string {
    return fmt::format("{}-{}", decoration_id, state);
}

}  // namespace logicsim
