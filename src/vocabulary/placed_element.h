#ifndef LOGICSIM_VOCABULARY_PLACED_ELEMENT_H
#define LOGICSIM_VOCABULARY_PLACED_ELEMENT_H

#include "format/struct.h"
#include "vocabulary/element_definition.h"
#include "vocabulary/point.h"

#include <compare>

namespace logicsim {

struct PlacedElement {
    ElementDefinition definition {};
    point_t position {0, 0};

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const PlacedElement& other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const PlacedElement&) const = default;
};

}  // namespace logicsim

#endif
