#ifndef LOGICSIM_VOCABULARY_PRINT_EVENTS_H
#define LOGICSIM_VOCABULARY_PRINT_EVENTS_H

#include "format/enum.h"

#include <string>

namespace logicsim {

enum class PrintEvents { yes, no };

template <>
auto format(PrintEvents type) -> std::string;

}  // namespace logicsim

#endif
