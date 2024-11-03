#ifndef LOGICSIM_CORE_VOCABULARY_SELECTION_FUNCTION_H
#define LOGICSIM_CORE_VOCABULARY_SELECTION_FUNCTION_H

#include "core/format/enum.h"

#include <string>

namespace logicsim {

enum class SelectionFunction : uint8_t {
    add,
    substract,
};

template <>
[[nodiscard]] auto format(SelectionFunction selection_function) -> std::string;

}  // namespace logicsim

#endif
