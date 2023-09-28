#ifndef LOGIKSIM_LAYOUT_CALCULATION_H
#define LOGIKSIM_LAYOUT_CALCULATION_H

#include "exception.h"
#include "glyph_cache_type.h"
#include "layout_calculation_type.h"
#include "range.h"
#include "vocabulary.h"

#include <array>
#include <vector>

struct BLPoint;

namespace logicsim {

// General
namespace defaults {
constexpr static inline auto line_selection_padding = 0.3;    // grid values
constexpr static inline auto logic_item_body_overdraw = 0.4;  // grid values
}  // namespace defaults

namespace standard_element {
constexpr static inline auto min_inputs = connection_count_t {2};
constexpr static inline auto max_inputs =
    connection_count_t {connection_id_t::max().value};
[[nodiscard]] auto height(connection_count_t input_count) -> grid_t;
constexpr static inline auto width = grid_t {2};
}  // namespace standard_element

// Display General
namespace display {
constexpr static inline auto font_style = FontStyle::monospace;
constexpr static inline auto font_size = grid_fine_t {0.9};  // grid values
constexpr static inline auto enable_input_id = connection_id_t {0};

constexpr static inline auto margin_horizontal = grid_fine_t {0.2};
constexpr static inline auto padding_vertical = grid_fine_t {0.7};
constexpr static inline auto padding_horizontal = grid_fine_t {0.25};
}  // namespace display

// Display Number
namespace display_number {
constexpr static inline auto control_inputs = connection_count_t {2};
[[nodiscard]] auto value_inputs(connection_count_t input_count) -> connection_count_t;
constexpr static inline auto min_value_inputs = connection_count_t {1};
constexpr static inline auto max_value_inputs = connection_count_t {64};
constexpr static inline auto min_inputs = control_inputs + min_value_inputs;
constexpr static inline auto max_inputs = control_inputs + max_value_inputs;
[[nodiscard]] auto width(connection_count_t input_count) -> grid_t;
[[nodiscard]] auto height(connection_count_t input_count) -> grid_t;

[[nodiscard]] auto input_shift(connection_count_t input_count) -> grid_t;
[[nodiscard]] auto enable_position(connection_count_t input_count) -> point_t;
[[nodiscard]] auto negative_position(connection_count_t input_count) -> point_t;
constexpr static inline auto negative_input_id = connection_id_t {1};
}  // namespace display_number

[[nodiscard]] auto is_input_output_count_valid(ElementType element_type,
                                               connection_count_t input_count,
                                               connection_count_t output_count) -> bool;
[[nodiscard]] auto is_orientation_valid(ElementType element_type,
                                        orientation_t orientation) -> bool;

[[nodiscard]] auto element_collision_rect(layout_calculation_data_t data) -> rect_t;
[[nodiscard]] auto element_selection_rect(layout_calculation_data_t data) -> rect_fine_t;
[[nodiscard]] auto element_selection_rect(ordered_line_t line) -> rect_fine_t;
[[nodiscard]] auto element_selection_rect_rounded(ordered_line_t line) -> rect_fine_t;
[[nodiscard]] auto element_bounding_rect(layout_calculation_data_t data) -> rect_t;

[[nodiscard]] auto is_representable(layout_calculation_data_t data) -> bool;
[[nodiscard]] auto orientations_compatible(orientation_t a, orientation_t b) -> bool;

auto require_min(connection_count_t value, connection_count_t count) -> void;
auto require_max(connection_count_t value, connection_count_t count) -> void;
auto require_equal(connection_count_t value, connection_count_t count) -> void;

[[nodiscard]] auto transform(point_t element_position, orientation_t orientation,
                             point_t offset) -> point_t;
[[nodiscard]] auto transform(point_t element_position, orientation_t orientation,
                             point_fine_t offset) -> point_fine_t;

[[nodiscard]] auto transform(orientation_t element_orientation, orientation_t connector)
    -> orientation_t;

auto connector_point(point_t position, orientation_t orientation, grid_fine_t offset)
    -> point_fine_t;
auto connector_point(BLPoint position, orientation_t orientation, double offset)
    -> BLPoint;

// Display ASCII
namespace display_ascii {
constexpr static inline auto control_inputs = connection_count_t {1};
constexpr static inline auto value_inputs = connection_count_t {7};
constexpr static inline auto input_count = control_inputs + value_inputs;
constexpr static inline auto width = grid_t {4};
constexpr static inline auto height =
    grid_t {(value_inputs - connection_count_t {1}).value};
constexpr static inline auto enable_position = point_t {2, height};
}  // namespace display_ascii

/// next_point(point_t position) -> bool;
auto iter_element_body_points(layout_calculation_data_t data,
                              std::invocable<point_t> auto next_point) -> bool {
    switch (data.element_type) {
        using enum ElementType;

        // without a body
        case unused:
        case placeholder:
        case wire:
        case buffer_element: {
            return true;
        }

        case and_element:
        case or_element:
        case xor_element: {
            require_min(data.input_count, standard_element::min_inputs);

            const auto height = data.input_count.value;
            const auto output_offset = (height - data.output_count.value) / 2;

            for (auto i : range(height)) {
                const auto y = grid_t {i};

                if (!next_point(
                        transform(data.position, data.orientation, point_t {1, y}))) {
                    return false;
                }

                if (i != output_offset) {
                    if (!next_point(
                            transform(data.position, data.orientation, point_t {2, y}))) {
                        return false;
                    }
                }
            }
            return true;
        }

        case led: {
            // has no body
            return true;
        }
        case button: {
            // has no body
            return true;
        }
        case display_number: {
            namespace display_number = logicsim::display_number;

            const auto width = display_number::width(data.input_count);
            const auto height = display_number::height(data.input_count);

            const auto negative_pos = display_number::negative_position(data.input_count);
            const auto enable_pos = display_number::enable_position(data.input_count);
            const auto max_input_y =
                grid_t {display_number::value_inputs(data.input_count).value} -
                grid_t {1};

            for (const auto y : range(int {height} + 1)) {
                for (const auto x : range(int {width} + 1)) {
                    const auto point = point_t {gsl::narrow_cast<grid_t::value_type>(x),
                                                gsl::narrow_cast<grid_t::value_type>(y)};

                    if (point.x == 0 && point.y <= max_input_y) {
                        continue;
                    }
                    if (point == negative_pos || point == enable_pos) {
                        continue;
                    }
                    if (!next_point(transform(data.position, data.orientation, point))) {
                        return false;
                    }
                }
            }
            return true;
        }
        case display_ascii: {
            namespace display_ascii = logicsim::display_ascii;

            for (const auto y : range(int {display_ascii::height} + 1)) {
                for (const auto x : range(1, int {display_ascii::width} + 1)) {
                    const auto point = point_t {gsl::narrow_cast<grid_t::value_type>(x),
                                                gsl::narrow_cast<grid_t::value_type>(y)};

                    if (point == display_ascii::enable_position) {
                        continue;
                    }
                    if (!next_point(transform(data.position, data.orientation, point))) {
                        return false;
                    }
                }
            }
            return true;
        }

        case clock_generator: {
            using p = point_t;
            auto points = std::array {
                p {0, 0}, p {1, 0}, p {2, 0}, p {3, 0}, p {4, 0}, p {5, 0},  //
                p {0, 1}, p {1, 1}, p {2, 1}, p {3, 1}, p {4, 1}, p {5, 1},  //
                p {0, 2}, p {1, 2}, p {2, 2}, p {3, 2}, p {4, 2},            //
                p {0, 3}, p {1, 3}, p {2, 3}, p {3, 3}, p {4, 3}, p {5, 3},  //
                p {0, 4}, p {1, 4}, p {2, 4}, p {4, 4}, p {5, 4},            //
            };

            for (auto &&point : points) {
                if (!next_point(transform(data.position, data.orientation, point))) {
                    return false;
                }
            }

            return true;
        }
        case flipflop_jk: {
            using p = point_t;
            auto points = std::array {p {1, 0}, p {1, 1}, p {1, 2},  //
                                      p {2, 1},                      //
                                      p {3, 0}, p {3, 1}, p {3, 2},  //
                                      p {4, 1}};

            for (auto &&point : points) {
                if (!next_point(transform(data.position, data.orientation, point))) {
                    return false;
                }
            }

            return true;
        }
        case shift_register: {
            require_min(data.input_count, connection_count_t {2});

            // TODO width depends on internal state
            const auto width = 2 * 4;
            const auto height =
                data.output_count <= connection_count_t {1}
                    ? grid_t {1}
                    : grid_t {((data.output_count - connection_count_t {1}) * 2).value};

            for (auto i : range(1, width)) {
                for (auto j : range(height + 1)) {
                    const auto x = grid_t {i};
                    const auto y = grid_t {j};
                    if (!next_point(
                            transform(data.position, data.orientation, point_t {x, y}))) {
                        return false;
                    }
                }
            }

            for (auto j = grid_t {1}; j < height; j = j + grid_t {2}) {
                const auto x = grid_t {width};
                const auto y = grid_t {j};
                if (!next_point(
                        transform(data.position, data.orientation, point_t {x, y}))) {
                    return false;
                }
            }
            return true;
        }

        case latch_d: {
            using p = point_t;
            auto points = std::array {p {1, 0}, p {1, 1}, p {2, 1}};

            for (auto &&point : points) {
                if (!next_point(transform(data.position, data.orientation, point))) {
                    return false;
                }
            }

            return true;
        }

        case flipflop_d: {
            using p = point_t;
            auto points = std::array {p {0, 2},                      //
                                      p {1, 0}, p {1, 1}, p {1, 2},  //
                                      p {2, 1},                      //
                                      p {3, 1}, p {3, 2}};

            for (auto &&point : points) {
                if (!next_point(transform(data.position, data.orientation, point))) {
                    return false;
                }
            }

            return true;
        }
        case flipflop_ms_d: {
            using p = point_t;
            auto points = std::array {p {0, 2},                      //
                                      p {1, 0}, p {1, 1}, p {1, 2},  //
                                      p {2, 1},                      //
                                      p {3, 0}, p {3, 1}, p {3, 2},  //
                                      p {4, 1}, p {4, 2}};

            for (auto &&point : points) {
                if (!next_point(transform(data.position, data.orientation, point))) {
                    return false;
                }
            }

            return true;
        }

        case sub_circuit: {
            throw_exception("not implemented");
        }
    }
    throw_exception("'Don't know to calculate input locations.");
}

// next_input(point_t position, orientation_t orientation) -> bool;
auto iter_input_location(layout_calculation_data_t data,
                         std::invocable<point_t, orientation_t> auto next_input) -> bool {
    switch (data.element_type) {
        using enum ElementType;

        case unused: {
            throw_exception("not supported");
        }

        case placeholder: {
            require_equal(data.input_count, connection_count_t {1});
            return next_input(data.position, orientation_t::undirected);
        }

        case wire: {
            throw_exception("not supported");
        }

        case buffer_element: {
            require_equal(data.input_count, connection_count_t {1});
            return next_input(data.position,
                              transform(data.orientation, orientation_t::left));
        }

        case and_element:
        case or_element:
        case xor_element: {
            require_min(data.input_count, standard_element::min_inputs);

            for (auto i : range(data.input_count.value)) {
                const auto y = grid_t {i};
                if (!next_input(
                        transform(data.position, data.orientation, point_t {0, y}),
                        transform(data.orientation, orientation_t::left))) {
                    return false;
                }
            }
            return true;
        }

        case led: {
            require_equal(data.input_count, connection_count_t {1});
            return next_input(data.position, orientation_t::undirected);
        }
        case button: {
            require_equal(data.input_count, connection_count_t {0});
            return true;
        }
        case display_number: {
            namespace display_number = logicsim::display_number;

            require_min(data.input_count, display_number::min_inputs);
            require_max(data.input_count, display_number::max_inputs);

            // enable
            static_assert(display::enable_input_id == connection_id_t {0});
            if (!next_input(transform(data.position, data.orientation,
                                      display_number::enable_position(data.input_count)),
                            transform(data.orientation, orientation_t::down))) {
                return false;
            }

            // negative
            static_assert(display_number::negative_input_id == connection_id_t {1});
            if (!next_input(
                    transform(data.position, data.orientation,
                              display_number::negative_position(data.input_count)),
                    transform(data.orientation, orientation_t::down))) {
                return false;
            }

            // number inputs
            for (auto i : range(display_number::value_inputs(data.input_count))) {
                const auto y = grid_t {i.value};
                if (!next_input(
                        transform(data.position, data.orientation, point_t {0, y}),
                        transform(data.orientation, orientation_t::left))) {
                    return false;
                }
            }
            return true;
        }
        case display_ascii: {
            namespace display_ascii = logicsim::display_ascii;
            require_equal(data.input_count, display_ascii::input_count);

            // enable
            static_assert(display::enable_input_id == connection_id_t {0});
            if (!next_input(transform(data.position, data.orientation,
                                      display_ascii::enable_position),
                            transform(data.orientation, orientation_t::down))) {
                return false;
            }

            // number inputs
            for (auto i : range(display_ascii::value_inputs)) {
                const auto y = grid_t {i.value};
                if (!next_input(
                        transform(data.position, data.orientation, point_t {0, y}),
                        transform(data.orientation, orientation_t::left))) {
                    return false;
                }
            }
            return true;
        }

        case clock_generator: {
            require_equal(data.input_count, connection_count_t {3});

            // the second input is used only for simulation
            // not for any drawing or any types of collisions

            return next_input(transform(data.position, data.orientation, point_t {3, 4}),
                              transform(data.orientation, orientation_t::down));
        }
        case flipflop_jk: {
            require_equal(data.input_count, connection_count_t {5});

            auto points = std::array {
                // clock
                std::make_pair(point_t {0, 1}, orientation_t::left),
                // j & k
                std::make_pair(point_t {0, 0}, orientation_t::left),
                std::make_pair(point_t {0, 2}, orientation_t::left),
                // set & reset
                std::make_pair(point_t {2, 0}, orientation_t::up),
                std::make_pair(point_t {2, 2}, orientation_t::down),
            };

            for (auto &&[point, orientation] : points) {
                if (!next_input(transform(data.position, data.orientation, point),
                                transform(data.orientation, orientation))) {
                    return false;
                }
            }
            return true;
        }
        case shift_register: {
            require_min(data.input_count, connection_count_t {2});

            // clock
            if (!next_input(transform(data.position, data.orientation, point_t {0, 1}),
                            transform(data.orientation, orientation_t::left))) {
                return false;
            }

            // memory row rows
            for (auto i : range(data.input_count - connection_count_t {1})) {
                const auto y = grid_t {2 * i.value};
                if (!next_input(
                        transform(data.position, data.orientation, point_t {0, y}),
                        transform(data.orientation, orientation_t::left))) {
                    return false;
                }
            }

            return true;
        }
        case latch_d: {
            require_min(data.input_count, connection_count_t {2});

            auto points = std::array {
                // clock
                std::make_pair(point_t {0, 1}, orientation_t::left),
                // data
                std::make_pair(point_t {0, 0}, orientation_t::left),
            };

            for (auto &&[point, orientation] : points) {
                if (!next_input(transform(data.position, data.orientation, point),
                                transform(data.orientation, orientation))) {
                    return false;
                }
            }
            return true;
        }
        case flipflop_d: {
            require_min(data.input_count, connection_count_t {4});

            auto points = std::array {
                // clock
                std::make_pair(point_t {0, 1}, orientation_t::left),
                // data
                std::make_pair(point_t {0, 0}, orientation_t::left),
                // set & reset
                std::make_pair(point_t {2, 0}, orientation_t::up),
                std::make_pair(point_t {2, 2}, orientation_t::down),
            };

            for (auto &&[point, orientation] : points) {
                if (!next_input(transform(data.position, data.orientation, point),
                                transform(data.orientation, orientation))) {
                    return false;
                }
            }
            return true;
        }
        case flipflop_ms_d: {
            require_min(data.input_count, connection_count_t {4});

            auto points = std::array {
                // clock
                std::make_pair(point_t {0, 1}, orientation_t::left),
                // data
                std::make_pair(point_t {0, 0}, orientation_t::left),
                // set & reset
                std::make_pair(point_t {2, 0}, orientation_t::up),
                std::make_pair(point_t {2, 2}, orientation_t::down),
            };

            for (auto &&[point, orientation] : points) {
                if (!next_input(transform(data.position, data.orientation, point),
                                transform(data.orientation, orientation))) {
                    return false;
                }
            }
            return true;
        }

        case sub_circuit: {
            throw_exception("not implemented");
        }
    }

    throw_exception("'Don't know to calculate input locations.");
}

// next_output(point_t position, orientation_t orientation) -> bool;
auto iter_output_location(layout_calculation_data_t data,
                          std::invocable<point_t, orientation_t> auto next_output)
    -> bool {
    switch (data.element_type) {
        using enum ElementType;

        case unused: {
            throw_exception("not supported");
        }

        case placeholder: {
            require_equal(data.output_count, connection_count_t {0});
            return true;
        }

        case wire: {
            throw_exception("not supported");
        }

        case buffer_element: {
            require_equal(data.output_count, connection_count_t {1});
            return next_output(transform(data.position, data.orientation, point_t {1, 0}),
                               transform(data.orientation, orientation_t::right));
        }

        case and_element:
        case or_element:
        case xor_element: {
            require_equal(data.output_count, connection_count_t {1});

            const auto height = data.input_count;
            const auto output_offset = grid_t {(height - data.output_count).value / 2};
            return next_output(
                transform(data.position, data.orientation, point_t {2, output_offset}),
                transform(data.orientation, orientation_t::right));
        }

        case led: {
            require_equal(data.output_count, connection_count_t {0});
            return true;
        }
        case button: {
            require_equal(data.output_count, connection_count_t {1});
            return next_output(data.position, data.orientation);
        }
        case display_number: {
            require_equal(data.output_count, connection_count_t {0});
            return true;
        }
        case display_ascii: {
            require_equal(data.output_count, connection_count_t {0});
            return true;
        }

        case clock_generator: {
            require_equal(data.output_count, connection_count_t {3});

            // the second output is used only for simulation
            // not for any drawing or any types of collisions

            return next_output(transform(data.position, data.orientation, point_t {5, 2}),
                               transform(data.orientation, orientation_t::right));
        }
        case flipflop_jk: {
            require_equal(data.output_count, connection_count_t {2});

            auto points = std::array {
                // Q and !Q
                std::make_pair(point_t {4, 0}, orientation_t::right),
                std::make_pair(point_t {4, 2}, orientation_t::right),
            };

            for (auto &&[point, orientation] : points) {
                if (!next_output(transform(data.position, data.orientation, point),
                                 transform(data.orientation, orientation))) {
                    return false;
                }
            }
            return true;
        }
        case shift_register: {
            require_min(data.output_count, connection_count_t {1});
            require_equal(data.output_count, data.input_count - connection_count_t {1});

            // TODO width depends on internal state
            const auto width = 2 * 4;

            // for each memory row
            for (auto i : range(data.output_count)) {
                const auto y = grid_t {2 * i.value};
                if (!next_output(
                        transform(data.position, data.orientation, point_t {width, y}),
                        transform(data.orientation, orientation_t::right))) {
                    return false;
                }
            }
            return true;
        }
        case latch_d: {
            require_equal(data.output_count, connection_count_t {1});

            // data
            return next_output(transform(data.position, data.orientation, point_t {2, 0}),
                               transform(data.orientation, orientation_t::right));
        }

        case flipflop_d: {
            require_equal(data.output_count, connection_count_t {1});

            // data
            return next_output(transform(data.position, data.orientation, point_t {3, 0}),
                               transform(data.orientation, orientation_t::right));
        }
        case flipflop_ms_d: {
            require_equal(data.output_count, connection_count_t {1});

            // data
            return next_output(transform(data.position, data.orientation, point_t {4, 0}),
                               transform(data.orientation, orientation_t::right));
        }

        case sub_circuit: {
            throw_exception("not implemented");
        }
    }
    throw_exception("'Don't know to calculate output locations.");
}

// next_input(connection_id_t input_id, point_t position,
//            orientation_t orientation) -> bool;
auto iter_input_location_and_id(
    layout_calculation_data_t data,
    std::invocable<connection_id_t, point_t, orientation_t> auto next_input) -> bool {
    return iter_input_location(
        data, [&, input_id = connection_id_t {0}](point_t position,
                                                  orientation_t orientation) mutable {
            return std::invoke(next_input, input_id++, position, orientation);
        });
}

// next_output(connection_id_t output_id, point_t position,
//             orientation_t orientation) -> bool;
auto iter_output_location_and_id(
    layout_calculation_data_t data,
    std::invocable<connection_id_t, point_t, orientation_t> auto next_output) -> bool {
    return iter_output_location(
        data, [&, output_id = connection_id_t {0}](point_t position,
                                                   orientation_t orientation) mutable {
            return std::invoke(next_output, output_id++, position, orientation);
        });
}

}  // namespace logicsim

#endif
