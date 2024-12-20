#ifndef LOGICSIM_VOCABULARY_PRINT_EVENTS_H
#define LOGICSIM_VOCABULARY_PRINT_EVENTS_H

#include "core/format/enum.h"

#include <string>

namespace logicsim {

enum class PrintEvents { yes, no };

template <>
[[nodiscard]] auto format(PrintEvents type) -> std::string;

}  // namespace logicsim

#endif
