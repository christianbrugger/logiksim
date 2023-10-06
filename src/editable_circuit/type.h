#ifndef LOGIKSIM_EDITABLE_CIRCUIT_TYPE_H
#define LOGIKSIM_EDITABLE_CIRCUIT_TYPE_H

#include "format/enum.h"
#include "layout.h"
#include "vocabulary.h"

#include <cstddef>
#include <optional>

namespace logicsim {

enum class LineInsertionType {
    horizontal_first,
    vertical_first,
};

template <>
[[nodiscard]] auto format(LineInsertionType type) -> std::string;

}  // namespace logicsim

#endif
