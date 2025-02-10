#ifndef LOGICSIM_CORE_VOCABULARY_PLACED_ELEMENT_H
#define LOGICSIM_CORE_VOCABULARY_PLACED_ELEMENT_H

#include "core/vocabulary/placed_decoration.h"
#include "core/vocabulary/placed_logicitem.h"

#include <fmt/core.h>

#include <variant>

namespace logicsim {

struct PlacedLogicItem;
struct PlacedDecoration;

using PlacedElement = std::variant<PlacedLogicItem, PlacedDecoration>;

}  // namespace logicsim

template <>
struct fmt::formatter<logicsim::PlacedElement> {
    constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    auto format(const logicsim::PlacedElement &obj, fmt::format_context &ctx) const;
};

#endif
