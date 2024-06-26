#include "vocabulary/drawable_element.h"

#include <fmt/core.h>

namespace logicsim {

auto DrawableElement::format() const -> std::string {
    return fmt::format("{}-{}", logicitem_id, state);
}

}  // namespace logicsim
