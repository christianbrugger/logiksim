#include "core/vocabulary/optional_logic_value.h"

#include <exception>

namespace logicsim {

template <>
auto format(OptionalLogicValue value) -> std::string {
    if (value.has_value()) {
        return *value ? "1" : "0";
    }
    return "?";
}

}  // namespace logicsim
