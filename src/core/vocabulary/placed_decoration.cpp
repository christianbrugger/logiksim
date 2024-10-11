#include "vocabulary/placed_decoration.h"

namespace logicsim {

auto PlacedDecoration::format() const -> std::string {
    return fmt::format("<{} {}>", position, definition);
}

}  // namespace logicsim
