#include "layout_calculations.h"

#include "geometry.h"

namespace logicsim {

auto require_min(std::size_t value, std::size_t count) -> void {
    if (value < count) [[unlikely]] {
        throw_exception("Object has not enough elements.");
    }
}

auto require_equal(std::size_t value, std::size_t count) -> void {
    if (value != count) [[unlikely]] {
        throw_exception("Object has wrong number of elements.");
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
        case undirected: {
            throw_exception("Cannot transform undirected elements.");
        }
    }
    throw_exception("Don't know how to transform position.");
}

auto transform(orientation_t element_orientation, orientation_t connector)
    -> orientation_t {
    switch (element_orientation) {
        using enum orientation_t;

        case right: {
            return connector;
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
        case undirected: {
            throw_exception("Cannot transform undirected element.");
        }
    }
    throw_exception("Don't know how to transform connector.");
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
        case undirected: {
            throw_exception("Cannot transform undirected element.");
        }
    }
    throw_exception("Don't know how to transform position.");
}

auto connector_endpoint(point_t position, orientation_t orientation) -> point_fine_t {
    const auto p0 = static_cast<point_fine_t>(position);
    const auto connector_offset = 0.4;

    switch (orientation) {
        using enum orientation_t;

        case right: {
            return {p0.x + connector_offset, p0.y};
        }
        case left: {
            return {p0.x - connector_offset, p0.y};
        }
        case up: {
            return {p0.x, p0.y - connector_offset};
        }
        case down: {
            return {p0.x, p0.y + connector_offset};
        }

        case undirected: {
            return p0;
        }
    };
    throw_exception("unknown orientation");
}

auto element_collision_rect(layout_calculation_data_t data) -> rect_t {
    switch (data.element_type) {
        using enum ElementType;

        case placeholder: {
            throw_exception("placeholder doesn't have a collision body");
        }

        case wire: {
            throw_exception("wire doesn't have a collision body");
        }

        case inverter_element: {
            return transform(data.position, data.orientation, {0, 0}, {1, 0});
        }

        case and_element:
        case or_element:
        case xor_element: {
            require_min(data.input_count, 1);
            const auto height = data.input_count;
            const auto y2 = grid_t {height - std::size_t {1}};
            return transform(data.position, data.orientation, {0, 0}, {2, y2});
        }

        case clock_generator: {
            return transform(data.position, data.orientation, {0, 0}, {3, 2});
        }
        case flipflop_jk: {
            return transform(data.position, data.orientation, {0, 0}, {4, 2});
        }
        case shift_register: {
            require_min(data.output_count, 1);

            // TODO width depends on internal state
            const auto width = 2 * 4;

            const auto x2 = grid_t {width};
            const auto y2 = data.output_count == 1 ? grid_t {1}
                                                   : grid_t {2 * (data.output_count - 1)};
            return transform(data.position, data.orientation, {0, 0}, {x2, y2});
        }
    }
    throw_exception("'Don't know to calculate collision rect.");
}

auto element_selection_rect(layout_calculation_data_t data) -> rect_fine_t {
    constexpr static auto overdraw = grid_fine_t {0.5};

    const auto rect = element_collision_rect(data);

    return rect_fine_t {
        point_fine_t {rect.p0.x.value - overdraw, rect.p0.y.value - overdraw},
        point_fine_t {rect.p1.x.value + overdraw, rect.p1.y.value + overdraw},
    };
}

auto element_selection_rect(line_t segment) -> rect_fine_t {
    constexpr auto width = grid_fine_t {0.3};

    const auto ordered_segment = order_points(segment);
    const auto p0 = static_cast<point_fine_t>(ordered_segment.p0);
    const auto p1 = static_cast<point_fine_t>(ordered_segment.p1);

    if (is_horizontal(segment)) {
        return rect_fine_t {
            point_fine_t {p0.x, p0.y - width},
            point_fine_t {p1.x, p1.y + width},
        };
    }
    if (is_vertical(segment)) {
        return rect_fine_t {
            point_fine_t {p0.x - width, p0.y},
            point_fine_t {p1.x + width, p1.y},
        };
    }
    return rect_fine_t {p0, p1};
}

}  // namespace logicsim