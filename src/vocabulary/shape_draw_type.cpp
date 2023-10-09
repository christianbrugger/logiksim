#include "vocabulary/shape_draw_type.h"

#include <exception>

namespace logicsim {

template <>
auto format(DrawType type) -> std::string {
    switch (type) {
        using enum DrawType;

        case fill:
            return "fill";
        case stroke:
            return "stroke";
        case fill_and_stroke:
            return "fill_and_stroke";
    }
    std::terminate();
}

auto do_fill(DrawType type) -> bool {
    using enum DrawType;
    return type == fill || type == fill_and_stroke;
}

auto do_stroke(DrawType type) -> bool {
    using enum DrawType;
    return type == stroke || type == fill_and_stroke;
}

}  // namespace logicsim
