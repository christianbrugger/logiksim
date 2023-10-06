#include "editable_circuit/type.h"

#include "exception.h"
#include "layout_calculation.h"

#include <fmt/core.h>

namespace logicsim {

template <>
auto format(LineInsertionType type) -> std::string {
    switch (type) {
        using enum LineInsertionType;

        case horizontal_first:
            return "horizontal_first";
        case vertical_first:
            return "vertical_first";
    }
    throw_exception("unknown LineInsertionType");
}

}  // namespace logicsim