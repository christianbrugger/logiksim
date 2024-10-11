#ifndef LOGICSIM_VOCABULARY_PLACED_ELEMENT_H
#define LOGICSIM_VOCABULARY_PLACED_ELEMENT_H

#include "format/struct.h"
#include "vocabulary/logicitem_definition.h"
#include "vocabulary/point.h"

#include <compare>
#include <type_traits>

namespace logicsim {

struct PlacedLogicItem {
    LogicItemDefinition definition {};
    point_t position {0, 0};

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const PlacedLogicItem& other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const PlacedLogicItem&) const = default;
};

static_assert(std::is_aggregate_v<PlacedLogicItem>);

}  // namespace logicsim

#endif
