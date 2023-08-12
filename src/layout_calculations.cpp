#include "layout_calculations.h"

#include "geometry.h"
#include "scene.h"

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
    const auto p0 = point_fine_t {position};
    const auto connector_offset = 0.4;

    switch (orientation) {
        using enum orientation_t;

        case right: {
            return point_fine_t {p0.x + connector_offset, p0.y};
        }
        case left: {
            return point_fine_t {p0.x - connector_offset, p0.y};
        }
        case up: {
            return point_fine_t {p0.x, p0.y - connector_offset};
        }
        case down: {
            return point_fine_t {p0.x, p0.y + connector_offset};
        }

        case undirected: {
            return p0;
        }
    };
    throw_exception("unknown orientation");
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

auto is_input_output_count_valid(ElementType element_type, std::size_t input_count,
                                 std::size_t output_count) -> bool {
    if (input_count > connection_id_t::max()) {
        return false;
    }
    if (output_count > connection_id_t::max()) {
        return false;
    }

    switch (element_type) {
        using enum ElementType;

        case unused: {
            return input_count == 0 && output_count == 0;
        }
        case placeholder: {
            return input_count == 1 && output_count == 0;
        }
        case wire: {
            return input_count <= 1 && output_count >= 1;
        }

        case buffer_element: {
            return input_count == 1 && output_count == 1;
        }
        case and_element:
        case or_element:
        case xor_element: {
            return input_count >= 2 && output_count == 1;
        }

        case button: {
            return input_count == 0 && output_count == 1;
        }
        case clock_generator: {
            return input_count == 2 && output_count == 2;
        }
        case flipflop_jk: {
            return input_count == 5 && output_count == 2;
        }
        case shift_register: {
            return input_count >= 2 && output_count >= 1 &&
                   input_count == output_count + 1;
        }
        case latch_d: {
            return input_count == 2 && output_count == 1;
        }
        case flipflop_d: {
            return input_count == 4 && output_count == 1;
        }
        case flipflop_ms_d: {
            return input_count == 4 && output_count == 1;
        }

        case sub_circuit: {
            return input_count > 0 || output_count > 0;
        }
    }

    throw_exception("invalid element");
}

[[nodiscard]] auto is_orientation_valid(ElementType element_type,
                                        orientation_t orientation) -> bool {
    if (element_type == ElementType::unused || element_type == ElementType::placeholder) {
        return true;
    }

    if (element_type == ElementType::button) {
        return orientation == orientation_t::undirected;
    }

    return orientation != orientation_t::undirected;
}

auto element_collision_rect(layout_calculation_data_t data) -> rect_t {
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

        case button: {
            return rect_t {data.position, data.position};
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
        case latch_d: {
            return transform(data.position, data.orientation, {0, 0}, {2, 1});
        }
        case flipflop_d: {
            return transform(data.position, data.orientation, {0, 0}, {3, 2});
        }
        case flipflop_ms_d: {
            return transform(data.position, data.orientation, {0, 0}, {4, 2});
        }

        case sub_circuit: {
            throw_exception("not implemented");
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

auto element_selection_rect(ordered_line_t line) -> rect_fine_t {
    constexpr auto padding = grid_fine_t {0.3};

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
    constexpr auto padding = grid_fine_t {0.3};

    const auto p0 = point_fine_t {line.p0};
    const auto p1 = point_fine_t {line.p1};

    return rect_fine_t {
        point_fine_t {p0.x - padding, p0.y - padding},
        point_fine_t {p1.x + padding, p1.y + padding},
    };
}

auto element_bounding_rect(layout_calculation_data_t data) -> rect_t {
    if (is_logic_item(data.element_type)) {
        return to_enclosing_rect(element_selection_rect(data));
    }
    throw_exception("Not supported for other types");
}

auto is_representable(layout_calculation_data_t data) -> bool {
    if (is_placeholder(data)) {
        return true;
    }
    if (data.element_type == ElementType::wire) {
        throw_exception("Not supported for wires.");
    }

    const auto position = data.position;
    data.position = point_t {0, 0};
    const auto rect = element_collision_rect(data);

    return is_representable(position.x.value + rect.p0.x.value,
                            position.y.value + rect.p0.y.value) &&
           is_representable(position.x.value + rect.p1.x.value,
                            position.y.value + rect.p1.y.value);
}

auto orientations_compatible(orientation_t a, orientation_t b) -> bool {
    using enum orientation_t;
    return (a == left && b == right) || (a == right && b == left) ||
           (a == up && b == down) || (a == down && b == up) || (a == undirected) ||
           (b == undirected);
}

}  // namespace logicsim