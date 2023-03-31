#ifndef LOGIKSIM_LAYOUT_CALCULATIONS_H
#define LOGIKSIM_LAYOUT_CALCULATIONS_H

#include "exceptions.h"
#include "iterator_adaptor.h"
#include "layout_calculation_type.h"
#include "range.h"
#include "vocabulary.h"

#include <array>
#include <vector>

namespace logicsim {
[[nodiscard]] auto element_collision_rect(layout_calculation_data_t data) -> rect_t;
[[nodiscard]] auto element_selection_rect(layout_calculation_data_t data) -> rect_fine_t;
[[nodiscard]] auto element_selection_rect(line_t segment) -> rect_fine_t;

auto require_min(std::size_t value, std::size_t count) -> void;
auto require_equal(std::size_t value, std::size_t count) -> void;

namespace detail {
[[nodiscard]] auto transform(point_t element_position, orientation_t orientation,
                             point_t offset) -> point_t;

[[nodiscard]] auto transform(orientation_t element_orientation, orientation_t connector)
    -> orientation_t;
}  // namespace detail

auto connector_endpoint(point_t position, orientation_t orientation) -> point_fine_t;

/// next_point(point_t position) -> bool;
template <typename Func>
auto iter_element_body_points(layout_calculation_data_t data, Func next_point) -> bool {
    using detail::transform;

    switch (data.element_type) {
        using enum ElementType;

        // without a body
        case placeholder:
        case wire:
        case inverter_element: {
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

        case clock_generator: {
            require_equal(data.input_count, 2);

            using p = point_t;
            auto points = std::array {p {0, 0}, p {1, 0}, p {2, 0}, p {3, 0},
                                      p {0, 1}, p {0, 2}, p {1, 2}, p {3, 2}};

            for (auto &&point : points) {
                if (!next_point(transform(data.position, data.orientation, point))) {
                    return false;
                }
            }

            return true;
        }
        case flipflop_jk: {
            require_equal(data.input_count, 5);

            using p = point_t;
            auto points = std::array {p {1, 0}, p {3, 0}, p {1, 1}, p {2, 1},
                                      p {3, 1}, p {4, 1}, p {1, 2}, p {3, 2}};

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
            const auto height = data.output_count == 1
                                    ? grid_t {1}
                                    : grid_t {2 * (data.output_count - std::size_t {1})};

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

            for (auto j = 1; j < height; j += 2) {
                const auto x = grid_t {width};
                const auto y = grid_t {j};
                if (!next_point(
                        transform(data.position, data.orientation, point_t {x, y}))) {
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

        case placeholder: {
            require_equal(data.input_count, 1);
            return next_input(data.position, orientation_t::undirected);
        }

        case wire: {
            throw_exception("not supported");
        }

        case inverter_element: {
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

        case clock_generator: {
            require_equal(data.input_count, 2);

            auto points = std::array {
                // internal input, placed such that it cannot be connected
                std::make_pair(point_t {1, 1}, orientation_t::down),
                // output clock signal
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

        case placeholder: {
            require_equal(data.output_count, 0);
            return true;
        }

        case wire: {
            throw_exception("not supported");
        }

        case inverter_element: {
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

        case clock_generator: {
            require_equal(data.output_count, 2);

            auto points = std::array {
                // internal input, placed such that it cannot be connected
                std::make_pair(point_t {2, 1}, orientation_t::up),
                // reset signal
                std::make_pair(point_t {3, 1}, orientation_t::right),
            };

            for (auto &&[point, orientation] : points) {
                if (!next_output(transform(data.position, data.orientation, point),
                                 transform(data.orientation, orientation))) {
                    return false;
                }
            }
            return true;
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