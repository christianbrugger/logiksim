#include "vocabulary/placed_element.h"

#include <fmt/core.h>

namespace logicsim {

auto PlacedElement::format() const -> std::string {
    return fmt::format("<{} {}>", position, definition);
}

}  // namespace logicsim
