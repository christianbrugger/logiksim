#include "layout_calculation.h"

#include "geometry.h"
#include "scene.h"
#include "timer.h"

#include <gcem.hpp>

namespace logicsim {

auto require_min(connection_count_t value, connection_count_t count) -> void {
    if (value < count) [[unlikely]] {
        throw_exception("Object has not enough elements.");
    }
}

auto require_max(connection_count_t value, connection_count_t count) -> void {
    if (value > count) [[unlikely]] {
        throw_exception("Object has not enough elements.");
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
            throw_exception("Cannot transform undirected elements.");
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

auto logicsim::standard_element::height(connection_count_t input_count) -> grid_t {
    require_min(input_count, min_inputs);
    return to_grid((input_count - connection_count_t {1}));
}

namespace display_number {
constexpr auto value_inputs_(connection_count_t input_count) -> connection_count_t {
    if (input_count < control_inputs) {
        throw_exception("input count too small");
    }
    return input_count - control_inputs;
}

auto value_inputs(connection_count_t input_count) -> connection_count_t {
    return value_inputs_(input_count);
}

namespace {

// WARNING: changing this function will make saves incompatible
constexpr auto _width(connection_count_t input_count) -> grid_t {
    using gcem::ceil;
    using gcem::floor;
    using gcem::log10;
    using gcem::max;

    // font dependent, gathered by running print_character_metrics()
    constexpr auto digit_size = 0.6;
    constexpr auto sign_width = 0.6;
    constexpr auto separator_width = 0.6;
    static_assert(display::font_style == FontStyle::monospace);

    // independent
    constexpr auto font_size = double {display::font_size};
    constexpr auto padding = double {display::padding_horizontal};
    constexpr auto margin = double {display::margin_horizontal};
    // lock in values we depend on
    static_assert(font_size == 0.9);
    static_assert(padding == 0.25);
    static_assert(margin == 0.2);

    const auto digit_count_2 = gsl::narrow<double>(value_inputs_(input_count).count());
    const auto digit_count_10 = ceil(max(1., digit_count_2) * log10(2));
    const auto digit_count_10_neg = ceil(max(1., digit_count_2 - 1.) * log10(2));

    // without sign
    const auto digit_width = [&](double digit_count_10_) {
        const auto separator_count_ = floor((digit_count_10_ - 1.) / 3.);
        return digit_count_10_ * digit_size + separator_count_ * separator_width;
    };

    const auto sign_effective_width = max(
        0., digit_width(digit_count_10_neg) + sign_width - digit_width(digit_count_10));

    const auto digit_width_grid =
        ceil((digit_width(digit_count_10) + sign_effective_width) * font_size +
             2 * padding + 2 * margin);

    return grid_t {gsl::narrow<grid_t::value_type>(max(3., 1. + digit_width_grid))};
}

constexpr auto _generate_widths() {
    constexpr auto count = std::size_t {max_inputs - min_inputs + connection_count_t {1}};
    auto result = std::array<grid_t::value_type, count> {};
    for (connection_count_t i = min_inputs; i <= max_inputs; ++i) {
        result[std::size_t {i - min_inputs}] = _width(i).value;
    }
    return result;
}

constexpr static inline auto generated_widths = _generate_widths();

// lock in generated values to make sure our saves are compatible
static_assert(generated_widths ==
              std::array<grid_t::value_type, 64> {
                  3,  3,  3,  3,  4,  4,  4,  5,  5,  5,  6,  6,  6,  6,  6,  6,   //
                  6,  7,  7,  7,  8,  8,  8,  8,  8,  8,  8,  9,  9,  9,  10, 10,  //
                  10, 10, 10, 10, 10, 11, 11, 12, 12, 12, 12, 12, 13, 13, 13, 13,  //
                  13, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16});

}  // namespace

auto width(connection_count_t input_count) -> grid_t {
    require_min(input_count, min_inputs);
    require_max(input_count, max_inputs);

    return grid_t {generated_widths.at((input_count - min_inputs).count())};
}

auto height(connection_count_t input_count) -> grid_t {
    require_min(input_count, min_inputs);
    require_max(input_count, max_inputs);

    return to_grid(
        std::max(connection_count_t {2}, input_count - connection_count_t {3}));
}

auto input_shift(connection_count_t input_count) -> grid_t {
    const auto space =
        width(input_count) - grid_t {1} - to_grid(display_number::control_inputs);
    return grid_t {(int {space.value} + 1) / 2};
}

auto negative_position(connection_count_t input_count) -> point_t {
    return point_t {grid_t {1} + input_shift(input_count), height(input_count)};
}

auto enable_position(connection_count_t input_count) -> point_t {
    return point_t {grid_t {2} + input_shift(input_count), height(input_count)};
}

}  // namespace display_number

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
                   output_count >= connection_count_t {1};
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
    if (element_type == ElementType::unused || element_type == ElementType::placeholder) {
        return true;
    }

    if (element_type == ElementType::button || element_type == ElementType::led) {
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

auto element_selection_rect(layout_calculation_data_t data) -> rect_fine_t {
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

auto element_bounding_rect(layout_calculation_data_t data) -> rect_t {
    if (is_logic_item(data.element_type)) {
        return enclosing_rect(element_selection_rect(data));
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
