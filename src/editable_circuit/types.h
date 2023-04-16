#ifndef LOGIKSIM_EDITABLE_CIRCUIT_TYPES_H
#define LOGIKSIM_EDITABLE_CIRCUIT_TYPES_H

#include "format.h"

#include <fmt/core.h>

namespace logicsim {

enum class LineSegmentType {
    horizontal_first,
    vertical_first,
};

template <>
[[nodiscard]] auto format(LineSegmentType type) -> std::string;

}  // namespace logicsim

#endif
