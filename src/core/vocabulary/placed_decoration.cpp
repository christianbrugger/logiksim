#include "core/vocabulary/placed_decoration.h"

#include "core/allocated_size/trait.h"

namespace logicsim {

auto PlacedDecoration::format() const -> std::string {
    return fmt::format("<{} {}>", position, definition);
}

auto PlacedDecoration::allocated_size() const -> std::size_t {
    return get_allocated_size(definition);
}

}  // namespace logicsim
