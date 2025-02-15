#ifndef LOGICSIM_VOCABULARY_PLACED_ELEMENT_H
#define LOGICSIM_VOCABULARY_PLACED_ELEMENT_H

#include "core/format/struct.h"
#include "core/vocabulary/logicitem_definition.h"
#include "core/vocabulary/point.h"

#include <compare>
#include <type_traits>

namespace logicsim {

struct PlacedLogicItem {
    LogicItemDefinition definition {};
    point_t position {0, 0};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto operator==(const PlacedLogicItem& other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const PlacedLogicItem&) const = default;
};

static_assert(std::is_aggregate_v<PlacedLogicItem>);

}  // namespace logicsim

#endif
