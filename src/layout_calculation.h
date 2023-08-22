#ifndef LOGIKSIM_LAYOUT_CALCULATION_H
#define LOGIKSIM_LAYOUT_CALCULATION_H

#include "exception.h"
#include "layout_calculation_type.h"
#include "vocabulary.h"

#include <array>
#include <vector>

struct BLPoint;

namespace logicsim {

namespace defaults {
constexpr static inline auto line_selection_padding = 0.3;
}

[[nodiscard]] auto is_input_output_count_valid(ElementType element_type,
                                               std::size_t input_count,
                                               std::size_t output_count) -> bool;
[[nodiscard]] auto is_orientation_valid(ElementType element_type,
                                        orientation_t orientation) -> bool;

[[nodiscard]] auto element_collision_rect(layout_calculation_data_t data) -> rect_t;
[[nodiscard]] auto element_selection_rect(layout_calculation_data_t data) -> rect_fine_t;
[[nodiscard]] auto element_selection_rect(ordered_line_t line) -> rect_fine_t;
[[nodiscard]] auto element_selection_rect_rounded(ordered_line_t line) -> rect_fine_t;
[[nodiscard]] auto element_bounding_rect(layout_calculation_data_t data) -> rect_t;

[[nodiscard]] auto is_representable(layout_calculation_data_t data) -> bool;
[[nodiscard]] auto orientations_compatible(orientation_t a, orientation_t b) -> bool;

auto require_min(std::size_t value, std::size_t count) -> void;
auto require_max(std::size_t value, std::size_t count) -> void;
auto require_equal(std::size_t value, std::size_t count) -> void;

namespace detail {
[[nodiscard]] auto transform(point_t element_position, orientation_t orientation,
                             point_t offset) -> point_t;

[[nodiscard]] auto transform(orientation_t element_orientation, orientation_t connector)
    -> orientation_t;
}  // namespace detail

// TODO !!! remove when not needed
auto connector_endpoint(point_t position, orientation_t orientation) -> point_fine_t;

auto connector_point(point_t position, orientation_t orientation, grid_fine_t offset)
    -> point_fine_t;
auto connector_point(BLPoint position, orientation_t orientation, double offset)
    -> BLPoint;

auto display_number_width(std::size_t input_count) -> grid_t;
auto display_number_height(std::size_t input_count) -> grid_t;

/// next_point(point_t position) -> bool;
template <typename Func>
auto iter_element_body_points(layout_calculation_data_t data, Func next_point) -> bool {
    using detail::transform;

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
            require_min(data.input_count, 2);

            const auto height = data.input_count;
            const auto output_offset = (height - data.output_count) / 2;

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
            require_min(data.input_count, 3);
            require_max(data.input_count, 66);

            const auto width = display_number_width(data.input_count);
            const auto height = display_number_height(data.input_count);

            for (const auto y : range(int {height.value} + 1)) {
                for (const auto x : range(int {width.value} + 1)) {
                    if (x == 0 && y < gsl::narrow_cast<int>(data.input_count) - 1) {
                        continue;
                    }
                    if ((x == 1 || x == 2) && y == gsl::narrow_cast<int>(height)) {
                        continue;
                    }
                    if (!next_point(transform(
                            data.position, data.orientation,
                            point_t {gsl::narrow_cast<grid_t::value_type>(x),
                                     gsl::narrow_cast<grid_t::value_type>(y)}))) {
                        return false;
                    }
                }
            }
            return true;
        }
        case display_ascii: {
            for (const auto y : range(6 + 1)) {
                for (const auto x : range(1, 4 + 1)) {
                    if (x == 2 && y == 6) {
                        continue;
                    }
                    if (!next_point(transform(
                            data.position, data.orientation,
                            point_t {gsl::narrow_cast<grid_t::value_type>(x),
                                     gsl::narrow_cast<grid_t::value_type>(y)}))) {
                        return false;
                    }
                }
            }
            return true;
        }

        case clock_generator: {
            using p = point_t;
            auto points = std::array {
                p {0, 0}, p {1, 0}, p {2, 0}, p {3, 0},  //
                p {0, 1}, p {1, 1}, p {2, 1},            //
                p {0, 2}, p {2, 2}, p {3, 2}             //
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
            require_min(data.input_count, 2);

            // TODO width depends on internal state
            const auto width = 2 * 4;
            const auto height =
                data.output_count <= 1 ? 1 : 2 * (data.output_count - std::size_t {1});

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

            for (auto j = std::size_t {1}; j < height; j += 2) {
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

/// next_input(point_t position, orientation_t orientation) -> bool;
template <typename Func>
auto iter_input_location(layout_calculation_data_t data, Func next_input) -> bool {
    using detail::transform;

    switch (data.element_type) {
        using enum ElementType;

        case unused: {
            throw_exception("not supported");
        }

        case placeholder: {
            require_equal(data.input_count, 1);
            return next_input(data.position, orientation_t::undirected);
        }

        case wire: {
            throw_exception("not supported");
        }

        case buffer_element: {
            require_equal(data.input_count, 1);
            return next_input(data.position,
                              transform(data.orientation, orientation_t::left));
        }

        case and_element:
        case or_element:
        case xor_element: {
            require_min(data.input_count, 2);

            for (auto i : range(data.input_count)) {
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
            require_equal(data.input_count, 1);
            return next_input(data.position, orientation_t::undirected);
        }
        case button: {
            require_equal(data.input_count, 0);
            return true;
        }
        case display_number: {
            require_min(data.input_count, 3);
            require_max(data.input_count, 66);

            // input enable
            {
                const auto y = display_number_height(data.input_count);
                if (!next_input(
                        transform(data.position, data.orientation, point_t {2, y}),
                        transform(data.orientation, orientation_t::down))) {
                    return false;
                }
                if (!next_input(
                        transform(data.position, data.orientation, point_t {1, y}),
                        transform(data.orientation, orientation_t::down))) {
                    return false;
                }
            }

            // 2^0 - 2^x
            for (auto i : range(data.input_count - std::size_t {2})) {
                const auto y = grid_t {i};
                if (!next_input(
                        transform(data.position, data.orientation, point_t {0, y}),
                        transform(data.orientation, orientation_t::left))) {
                    return false;
                }
            }
            return true;
        }
        case display_ascii: {
            require_equal(data.input_count, 8);
            if (!next_input(transform(data.position, data.orientation, point_t {2, 6}),
                            transform(data.orientation, orientation_t::down))) {
                return false;
            }

            for (auto i : range(7)) {
                const auto y = grid_t {i};
                if (!next_input(
                        transform(data.position, data.orientation, point_t {0, y}),
                        transform(data.orientation, orientation_t::left))) {
                    return false;
                }
            }
            return true;
        }

        case clock_generator: {
            require_equal(data.input_count, 2);

            // the second input is used only for simulation
            // not for any drawing or any types of collisions

            return next_input(transform(data.position, data.orientation, point_t {1, 2}),
                              transform(data.orientation, orientation_t::down));
        }
        case flipflop_jk: {
            require_equal(data.input_count, 5);

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
            require_min(data.input_count, 2);

            // clock
            if (!next_input(transform(data.position, data.orientation, point_t {0, 1}),
                            transform(data.orientation, orientation_t::left))) {
                return false;
            }

            // memory row rows
            for (auto i : range(data.input_count - 1)) {
                const auto y = grid_t {2 * i};
                if (!next_input(
                        transform(data.position, data.orientation, point_t {0, y}),
                        transform(data.orientation, orientation_t::left))) {
                    return false;
                }
            }

            return true;
        }
        case latch_d: {
            require_min(data.input_count, 2);

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
            require_min(data.input_count, 4);

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
            require_min(data.input_count, 4);

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

/// next_output(point_t position, orientation_t orientation) -> bool;
template <typename Func>
auto iter_output_location(layout_calculation_data_t data, Func next_output) -> bool {
    using detail::transform;

    switch (data.element_type) {
        using enum ElementType;

        case unused: {
            throw_exception("not supported");
        }

        case placeholder: {
            require_equal(data.output_count, 0);
            return true;
        }

        case wire: {
            throw_exception("not supported");
        }

        case buffer_element: {
            require_equal(data.output_count, 1);
            return next_output(transform(data.position, data.orientation, point_t {1, 0}),
                               transform(data.orientation, orientation_t::right));
        }

        case and_element:
        case or_element:
        case xor_element: {
            require_equal(data.output_count, 1);

            const auto height = data.input_count;
            const auto output_offset = grid_t {(height - data.output_count) / 2};
            return next_output(
                transform(data.position, data.orientation, point_t {2, output_offset}),
                transform(data.orientation, orientation_t::right));
        }

        case led: {
            require_equal(data.output_count, 0);
            return true;
        }
        case button: {
            require_equal(data.output_count, 1);
            return next_output(data.position, data.orientation);
        }
        case display_number: {
            require_equal(data.output_count, 0);
            return true;
        }
        case display_ascii: {
            require_equal(data.output_count, 0);
            return true;
        }

        case clock_generator: {
            require_equal(data.output_count, 2);

            // the second output is used only for simulation
            // not for any drawing or any types of collisions

            return next_output(transform(data.position, data.orientation, point_t {3, 1}),
                               transform(data.orientation, orientation_t::right));
        }
        case flipflop_jk: {
            require_equal(data.output_count, 2);

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
            require_min(data.output_count, 1);
            require_equal(data.output_count, data.input_count - 1);

            // TODO width depends on internal state
            const auto width = 2 * 4;

            // for each memory row
            for (auto i : range(data.output_count)) {
                const auto y = grid_t {2 * i};
                if (!next_output(
                        transform(data.position, data.orientation, point_t {width, y}),
                        transform(data.orientation, orientation_t::right))) {
                    return false;
                }
            }
            return true;
        }
        case latch_d: {
            require_equal(data.output_count, 1);

            // data
            return next_output(transform(data.position, data.orientation, point_t {2, 0}),
                               transform(data.orientation, orientation_t::right));
        }

        case flipflop_d: {
            require_equal(data.output_count, 1);

            // data
            return next_output(transform(data.position, data.orientation, point_t {3, 0}),
                               transform(data.orientation, orientation_t::right));
        }
        case flipflop_ms_d: {
            require_equal(data.output_count, 1);

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
template <typename Func>
auto iter_input_location_and_id(layout_calculation_data_t data, Func next_input) -> bool {
    return iter_input_location(
        data, [&, input_id = connection_id_t {0}](point_t position,
                                                  orientation_t orientation) mutable {
            return next_input(input_id++, position, orientation);
        });
}

// next_output(connection_id_t output_id, point_t position,
//             orientation_t orientation) -> bool;
template <typename Func>
auto iter_output_location_and_id(layout_calculation_data_t data, Func next_output)
    -> bool {
    return iter_output_location(
        data, [&, output_id = connection_id_t {0}](point_t position,
                                                   orientation_t orientation) mutable {
            return next_output(output_id++, position, orientation);
        });
}

}  // namespace logicsim

#endif