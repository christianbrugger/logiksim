#include "layout_calculations.h"

namespace logicsim {

auto require_min_input_count(Schematic::ConstElement element, std::size_t count) -> void {
    if (element.input_count() < count) [[unlikely]] {
        throw_exception("Element has not enough inputs.");
    }
}

auto require_min_output_count(Schematic::ConstElement element, std::size_t count)
    -> void {
    if (element.output_count() < count) [[unlikely]] {
        throw_exception("Element has not enough outputs.");
    }
}

auto require_input_count(Schematic::ConstElement element, std::size_t count) -> void {
    if (element.input_count() != count) [[unlikely]] {
        throw_exception("Element has wrong number of inputs.");
    }
}

auto require_output_count(Schematic::ConstElement element, std::size_t count) -> void {
    if (element.output_count() != count) [[unlikely]] {
        throw_exception("Element has wrong number of outputs.");
    }
}

auto transform(point_t element_position, DisplayOrientation orientation, point_t offset)
    -> point_t {
    switch (orientation) {
        using enum DisplayOrientation;

        case default_right: {
            return element_position + offset;
        }
    }
    throw_exception("'Don't know to transfor mlocations.");
}

}  // namespace logicsim