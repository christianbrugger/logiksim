#ifndef LOGICSIM_CORE_VOCABULARY_PLACED_DECORATION_H
#define LOGICSIM_CORE_VOCABULARY_PLACED_DECORATION_H

#include "format/struct.h"
#include "vocabulary/decoration_definition.h"
#include "vocabulary/point.h"

#include <compare>
#include <type_traits>

namespace logicsim {

struct PlacedDecoration {
    DecorationDefinition definition {};
    point_t position {0, 0};

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const PlacedDecoration& other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const PlacedDecoration&) const = default;
};

static_assert(std::is_aggregate_v<PlacedDecoration>);

}  // namespace logicsim

#endif
