#ifndef LOGICSIM_VOCABULARY_LINE_INSERTION_TYPE_H
#define LOGICSIM_VOCABULARY_LINE_INSERTION_TYPE_H

#include "core/format/enum.h"

#include <string>

namespace logicsim {

enum class LineInsertionType {
    horizontal_first,
    vertical_first,
};

template <>
[[nodiscard]] auto format(LineInsertionType type) -> std::string;

}  // namespace logicsim

#endif
