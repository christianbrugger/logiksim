#include "layout_calculation.h"

#include "geometry/grid.h"
#include "geometry/orientation.h"
#include "geometry/rect.h"

#include <blend2d.h>
#include <gcem.hpp>

namespace logicsim {

auto require_min(connection_count_t value, connection_count_t count) -> void {
    if (value < count) [[unlikely]] {
        throw_exception("Object has not enough elements.");
    }
}

auto require_max(connection_count_t value, connection_count_t count) -> void {
    if (value > count) [[unlikely]] {
        throw_exception("Object has too many elements.");
    }
}

auto require_equal(connection_count_t value, connection_count_t count) -> void {
    if (value != count) [[unlikely]] {
        throw_exception("Object has wrong number of elements.");
    }
}

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
            return element_position + offset;
            // throw_exception("Cannot transform undirected elements.");
        }
    }
    throw_exception("Don't know how to transform position.");
}

auto transform(point_t element_position, orientation_t orientation, point_fine_t offset)
    -> point_fine_t {
    switch (orientation) {
        using enum orientation_t;

        case right: {
            return point_fine_t {element_position} + offset;
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
            return point_fine_t {element_position} + offset;
            // throw_exception("Cannot transform undirected elements.");
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
            return connector;
            // throw_exception("Cannot transform undirected element.");
        }
    }
    throw_exception("Don't know how to transform connector.");
}

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
            return rect_t {position + p0, position + p1};
            // throw_exception("Cannot transform undirected element.");
        }
    }
    throw_exception("Don't know how to transform position.");
}

auto connector_point(point_t position, orientation_t orientation, grid_fine_t offset)
    -> point_fine_t {
    const auto p0 = point_fine_t {position};

    switch (orientation) {
        using enum orientation_t;

        case right: {
            return point_fine_t {p0.x + offset, p0.y};
        }
        case left: {
            return point_fine_t {p0.x - offset, p0.y};
        }
        case up: {
            return point_fine_t {p0.x, p0.y - offset};
        }
        case down: {
            return point_fine_t {p0.x, p0.y + offset};
        }

        case undirected: {
            return p0;
        }
    };
    throw_exception("unknown orientation");
}

auto connector_point(BLPoint position, orientation_t orientation, double offset)
    -> BLPoint {
    switch (orientation) {
        using enum orientation_t;

        case right: {
            return BLPoint {position.x + offset, position.y};
        }
        case left: {
            return BLPoint {position.x - offset, position.y};
        }
        case up: {
            return BLPoint {position.x, position.y - offset};
        }
        case down: {
            return BLPoint {position.x, position.y + offset};
        }

        case undirected: {
            return position;
        }
    };
    throw_exception("unknown orientation");
}

auto is_input_output_count_valid(ElementType element_type, connection_count_t input_count,
                                 connection_count_t output_count) -> bool {
    switch (element_type) {
        using enum ElementType;

        case unused: {
            return input_count == connection_count_t {0} &&
                   output_count == connection_count_t {0};
        }
        case placeholder: {
            return input_count == connection_count_t {1} &&
                   output_count == connection_count_t {0};
        }
        case wire: {
            return input_count <= connection_count_t {1} &&
                   output_count >= connection_count_t {0};
        }

        case buffer_element: {
            return input_count == connection_count_t {1} &&
                   output_count == connection_count_t {1};
        }
        case and_element:
        case or_element:
        case xor_element: {
            return input_count >= standard_element::min_inputs &&
                   input_count <= standard_element::max_inputs &&
                   output_count == connection_count_t {1};
        }

        case button: {
            return input_count == connection_count_t {0} &&
                   output_count == connection_count_t {1};
        }
        case led: {
            return input_count == connection_count_t {1} &&
                   output_count == connection_count_t {0};
        }
        case display_number: {
            return input_count >= display_number::min_inputs &&
                   input_count <= display_number::max_inputs &&
                   output_count == connection_count_t {0};
        }
        case display_ascii: {
            return input_count == display_ascii::input_count &&
                   output_count == connection_count_t {0};
        }

        case clock_generator: {
            return input_count == connection_count_t {3} &&
                   output_count == connection_count_t {3};
        }
        case flipflop_jk: {
            return input_count == connection_count_t {5} &&
                   output_count == connection_count_t {2};
        }
        case shift_register: {
            return input_count >= connection_count_t {2} &&
                   output_count >= connection_count_t {1} &&
                   input_count == output_count + connection_count_t {1};
        }
        case latch_d: {
            return input_count == connection_count_t {2} &&
                   output_count == connection_count_t {1};
        }
        case flipflop_d: {
            return input_count == connection_count_t {4} &&
                   output_count == connection_count_t {1};
        }
        case flipflop_ms_d: {
            return input_count == connection_count_t {4} &&
                   output_count == connection_count_t {1};
        }

        case sub_circuit: {
            return input_count > connection_count_t {0} ||
                   output_count > connection_count_t {0};
        }
    }

    throw_exception("invalid element");
}

[[nodiscard]] auto is_orientation_valid(ElementType element_type,
                                        orientation_t orientation) -> bool {
    if (element_type == ElementType::unused || element_type == ElementType::placeholder ||
        element_type == ElementType::wire) {
        return true;
    }

    if (element_type == ElementType::button || element_type == ElementType::led) {
        return orientation == orientation_t::undirected;
    }

    return orientation != orientation_t::undirected;
}

auto element_collision_rect(const layout_calculation_data_t &data) -> rect_t {
    switch (data.element_type) {
        using enum ElementType;

        case unused: {
            throw_exception("unused doesn't have a collision body");
        }
        case placeholder: {
            throw_exception("placeholder doesn't have a collision body");
        }

        case wire: {
            throw_exception("not supported");
        }

        case buffer_element: {
            return transform(data.position, data.orientation, point_t {0, 0},
                             point_t {1, 0});
        }

        case and_element:
        case or_element:
        case xor_element: {
            require_min(data.input_count, standard_element::min_inputs);

            const auto height = data.input_count;
            const auto y2 = to_grid(height - connection_count_t {1});
            return transform(data.position, data.orientation, point_t {0, 0},
                             point_t {2, y2});
        }

        case led: {
            return rect_t {data.position, data.position};
        }
        case button: {
            return rect_t {data.position, data.position};
        }
        case display_number: {
            const auto w = display_number::width(data.input_count);
            const auto h = display_number::height(data.input_count);

            return transform(data.position, data.orientation, point_t {0, 0},
                             point_t {w, h});
        }
        case display_ascii: {
            const auto w = display_ascii::width;
            const auto h = display_ascii::height;

            return transform(data.position, data.orientation, point_t {0, 0},
                             point_t {w, h});
        }

        case clock_generator: {
            return transform(data.position, data.orientation, point_t {0, 0},
                             point_t {5, 4});
        }
        case flipflop_jk: {
            return transform(data.position, data.orientation, point_t {0, 0},
                             point_t {4, 2});
        }
        case shift_register: {
            require_min(data.output_count, connection_count_t {1});

            // TODO width depends on internal state
            const auto width = 2 * 4;

            const auto x2 = grid_t {width};
            const auto y2 = data.output_count == connection_count_t {1}
                                ? grid_t {1}
                                : to_grid(data.output_count - connection_count_t {1}) * 2;
            return transform(data.position, data.orientation, point_t {0, 0},
                             point_t {x2, y2});
        }
        case latch_d: {
            return transform(data.position, data.orientation, point_t {0, 0},
                             point_t {2, 1});
        }
        case flipflop_d: {
            return transform(data.position, data.orientation, point_t {0, 0},
                             point_t {3, 2});
        }
        case flipflop_ms_d: {
            return transform(data.position, data.orientation, point_t {0, 0},
                             point_t {4, 2});
        }

        case sub_circuit: {
            throw_exception("not implemented");
        }
    }
    throw_exception("'Don't know to calculate collision rect.");
}

auto element_selection_rect(const layout_calculation_data_t &data) -> rect_fine_t {
    constexpr static auto overdraw = grid_fine_t {0.5};

    const auto rect = element_collision_rect(data);

    return rect_fine_t {
        point_fine_t {rect.p0.x - overdraw, rect.p0.y - overdraw},
        point_fine_t {rect.p1.x + overdraw, rect.p1.y + overdraw},
    };
}

auto element_selection_rect(ordered_line_t line) -> rect_fine_t {
    constexpr auto padding = grid_fine_t {defaults::line_selection_padding};

    const auto p0 = point_fine_t {line.p0};
    const auto p1 = point_fine_t {line.p1};

    if (is_horizontal(line)) {
        return rect_fine_t {
            point_fine_t {p0.x, p0.y - padding},
            point_fine_t {p1.x, p1.y + padding},
        };
    }
    if (is_vertical(line)) {
        return rect_fine_t {
            point_fine_t {p0.x - padding, p0.y},
            point_fine_t {p1.x + padding, p1.y},
        };
    }
    return rect_fine_t {p0, p1};
}

auto element_selection_rect_rounded(ordered_line_t line) -> rect_fine_t {
    constexpr auto padding = grid_fine_t {defaults::line_selection_padding};

    const auto p0 = point_fine_t {line.p0};
    const auto p1 = point_fine_t {line.p1};

    return rect_fine_t {
        point_fine_t {p0.x - padding, p0.y - padding},
        point_fine_t {p1.x + padding, p1.y + padding},
    };
}

auto element_bounding_rect(const layout_calculation_data_t &data) -> rect_t {
    if (is_logic_item(data.element_type)) {
        return enclosing_rect(element_selection_rect(data));
    }
    throw_exception("Not supported for other types");
}

auto is_representable(layout_calculation_data_t data) -> bool {
    if (!is_logic_item(data.element_type)) {
        throw_exception("Only supported for logic items.");
    }

    const auto position = data.position;
    data.position = point_t {0, 0};
    const auto rect = element_collision_rect(data);

    static_assert(sizeof(int) > sizeof(grid_t::value_type));
    return is_representable(int {position.x} + int {rect.p0.x},
                            int {position.y} + int {rect.p0.y}) &&
           is_representable(int {position.x} + int {rect.p1.x},
                            int {position.y} + int {rect.p1.y});
}

auto orientations_compatible(orientation_t a, orientation_t b) -> bool {
    using enum orientation_t;
    return (a == left && b == right) || (a == right && b == left) ||
           (a == up && b == down) || (a == down && b == up) || (a == undirected) ||
           (b == undirected);
}

}  // namespace logicsim
