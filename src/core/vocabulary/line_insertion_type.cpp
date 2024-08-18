#include "vocabulary/line_insertion_type.h"

#include <exception>

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
    std::terminate();
}

}  // namespace logicsim
