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

namespace detail {

auto transform(point_t element_position, orientation_t orientation, point_t offset)
    -> point_t {
    switch (orientation) {
        using enum orientation_t;

        case right: {
            return element_position + offset;
        }
        case left: {
            throw_exception("Please implement.");
        }
        case up: {
            throw_exception("Please implement.");
        }
        case down: {
            throw_exception("Please implement.");
        }
    }
    throw_exception("Don't know how to transform locations.");
}

}  // namespace detail

auto transform(point_t position, orientation_t orientation, point_t p0, point_t p1)
    -> rect_t {
    switch (orientation) {
        using enum orientation_t;

        case right: {
            return rect_t {position + p0, position + p1};
        }
        case left: {
            throw_exception("Please implement.");
        }
        case up: {
            throw_exception("Please implement.");
        }
        case down: {
            throw_exception("Please implement.");
        }
    }
    throw_exception("Don't know how to transform locations.");
}

auto element_collision_rect(const Schematic &schematic, const Layout &layout,
                            element_id_t element_id) -> rect_t {
    const auto element = schematic.element(element_id);
    const auto orientation = layout.orientation(element_id);

    switch (element.element_type()) {
        using enum ElementType;

        case placeholder: {
            throw_exception("placeholder doesn't have a collision body");
        }

        case wire: {
            throw_exception("wire doesn't have a collision body");
        }

        case inverter_element: {
            const auto position = layout.position(element_id);
            return transform(position, orientation, {0, 0}, {1, 0});
        }

        case and_element:
        case or_element:
        case xor_element: {
            require_min_input_count(element, 1);
            const auto position = layout.position(element_id);
            const auto height = element.input_count();
            const auto y2 = grid_t {height - std::size_t {1}};
            return transform(position, orientation, {0, 0}, {2, y2});
        }

        case clock_generator: {
            const auto position = layout.position(element_id);
            return transform(position, orientation, {0, 0}, {3, 2});
        }
        case flipflop_jk: {
            const auto position = layout.position(element_id);
            return transform(position, orientation, {0, 0}, {4, 2});
        }
        case shift_register: {
            require_min_output_count(element, 1);
            const auto position = layout.position(element_id);
            const auto output_count = element.output_count();

            // TODO width depends on internal state
            const auto width = 2 * 4;

            const auto x2 = grid_t {width};
            const auto y2 = output_count == 1
                                ? grid_t {1}
                                : grid_t {2 * (element.output_count() - std::size_t {1})};
            return transform(position, orientation, {0, 0}, {x2, y2});
        }
    }
    throw_exception("'Don't know to calculate collision rect.");
}

}  // namespace logicsim