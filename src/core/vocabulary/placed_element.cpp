#include "core/vocabulary/placed_element.h"

#include "core/vocabulary/placed_decoration.h"
#include "core/vocabulary/placed_logicitem.h"

auto fmt::formatter<logicsim::PlacedElement>::format(const logicsim::PlacedElement &obj,
                                                     fmt::format_context &ctx) const {
    const auto str = std::visit([](auto &&v) { return v.format(); }, obj);
    return fmt::format_to(ctx.out(), "{}", str);
}
