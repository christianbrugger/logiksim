#include "editable_circuit/types.h"

#include "exceptions.h"

namespace logicsim {

auto format(LineSegmentType type) -> std::string {
    switch (type) {
        using enum LineSegmentType;

        case horizontal_first:
            return "horizontal_first";
        case vertical_first:
            return "vertical_first";
    }
    throw_exception("unknown LineSegmentType");
}

}  // namespace logicsim