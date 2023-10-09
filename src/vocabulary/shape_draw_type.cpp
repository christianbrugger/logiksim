#include "vocabulary/shape_draw_type.h"

#include <exception>

namespace logicsim {

template <>
auto format(ShapeDrawType type) -> std::string {
    switch (type) {
        using enum ShapeDrawType;

        case fill:
            return "fill";
        case stroke:
            return "stroke";
        case fill_and_stroke:
            return "fill_and_stroke";
    }
    std::terminate();
}

auto do_fill(ShapeDrawType type) -> bool {
    using enum ShapeDrawType;
    return type == fill || type == fill_and_stroke;
}

auto do_stroke(ShapeDrawType type) -> bool {
    using enum ShapeDrawType;
    return type == stroke || type == fill_and_stroke;
}

}  // namespace logicsim
