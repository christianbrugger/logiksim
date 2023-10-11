#include "layout_calculation.h"

#include "geometry/grid.h"
#include "geometry/orientation.h"
#include "geometry/rect.h"

#include <blend2d.h>
#include <gcem.hpp>

namespace logicsim {

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
    const auto info = get_layout_info(element_type);

    return info.input_count_min <= input_count && input_count <= info.input_count_max &&
           info.output_count_min <= output_count && output_count <= info.output_count_max;
}

auto is_orientation_valid(ElementType element_type, orientation_t orientation) -> bool {
    const auto info = get_layout_info(element_type);

    switch (info.direction_type) {
        using enum DirectionType;
        case undirected:
            return orientation == orientation_t::undirected;
        case directed:
            return orientation != orientation_t::undirected;
        case any:
            return true;
    }
    std::terminate();
}

auto element_collision_rect(const layout_calculation_data_t &data) -> rect_t {
    const auto info = get_layout_info(data.element_type);

    const auto width = info.variable_width ? info.variable_width(data) : info.fixed_width;
    const auto height =
        info.variable_height ? info.variable_height(data) : info.fixed_height;

    return transform(data.position, data.orientation, point_t {0, 0},
                     point_t {width, height});
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
