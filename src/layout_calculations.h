#ifndef LOGIKSIM_LAYOUT_CALCULATIONS_H
#define LOGIKSIM_LAYOUT_CALCULATIONS_H

#include "exceptions.h"
#include "layout_calculation_type.h"
#include "range.h"
#include "vocabulary.h"

#include <array>
#include <vector>

namespace logicsim {
[[nodiscard]] auto element_collision_rect(layout_calculation_data_t data) -> rect_t;

auto require_min(std::size_t value, std::size_t count) -> void;
auto require_equal(std::size_t value, std::size_t count) -> void;

namespace detail {
[[nodiscard]] auto transform(point_t element_position, orientation_t orientation,
                             point_t offset) -> point_t;
}  // namespace detail

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
    }
    throw_exception("'Don't know to calculate input locations.");
}

/// next_input(point_t position) -> bool;
template <typename Func>
auto iter_input_location(layout_calculation_data_t data, Func next_input) -> bool {
    using detail::transform;

    switch (data.element_type) {
        using enum ElementType;

        case placeholder: {
            require_equal(data.input_count, 1);
            return next_input(data.position);
        }

        case wire: {
            require_equal(data.input_count, 1);
            return next_input(data.line_tree.input_position());
        }

        case inverter_element: {
            require_equal(data.input_count, 1);
            return next_input(data.position);
        }

        case and_element:
        case or_element:
        case xor_element: {
            require_min(data.input_count, 2);

            for (auto i : range(data.input_count)) {
                const auto y = grid_t {i};
                if (!next_input(
                        transform(data.position, data.orientation, point_t {0, y}))) {
                    return false;
                }
            }
            return true;
        }

        case clock_generator: {
            require_equal(data.input_count, 2);

            auto points = std::array {
                // internal input, placed such that it cannot be connected
                point_t {1, 1},
                // output signal
                point_t {2, 2},
            };

            for (auto &&point : points) {
                if (!next_input(transform(data.position, data.orientation, point))) {
                    return false;
                }
            }
            return true;
        }
        case flipflop_jk: {
            require_equal(data.input_count, 5);

            auto points = std::array {
                // clock
                point_t {0, 1},
                // j & k
                point_t {0, 0},
                point_t {0, 2},
                // set & reset
                point_t {2, 0},
                point_t {2, 2},
            };

            for (auto &&point : points) {
                if (!next_input(transform(data.position, data.orientation, point))) {
                    return false;
                }
            }
            return true;
        }
        case shift_register: {
            require_min(data.input_count, 2);

            // clock
            if (!next_input(transform(data.position, data.orientation, point_t {0, 1}))) {
                return false;
            }

            // memory row rows
            for (auto i : range(data.input_count - 1)) {
                const auto y = grid_t {2 * i};
                if (!next_input(
                        transform(data.position, data.orientation, point_t {0, y}))) {
                    return false;
                }
            }

            return true;
        }
    }
    throw_exception("'Don't know to calculate input locations.");
}

/// next_output(point_t position) -> void;
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
            require_min(data.output_count, 1);
            require_equal(data.output_count, data.line_tree.output_count());

            for (auto &&point : data.line_tree.output_positions()) {
                if (!next_output(point)) {
                    return false;
                }
            }
            return true;
        }

        case inverter_element: {
            require_equal(data.output_count, 1);
            return next_output(
                transform(data.position, data.orientation, point_t {1, 0}));
        }

        case and_element:
        case or_element:
        case xor_element: {
            require_equal(data.output_count, 1);

            const auto height = data.input_count;
            const auto output_offset = grid_t {(height - data.output_count) / 2};
            return next_output(
                transform(data.position, data.orientation, point_t {2, output_offset}));
        }

        case clock_generator: {
            require_equal(data.output_count, 2);

            auto points = std::array {
                // internal input, placed such that it cannot be connected
                point_t {2, 1},
                // reset signal
                point_t {3, 1},
            };

            for (auto &&point : points) {
                if (!next_output(transform(data.position, data.orientation, point))) {
                    return false;
                }
            }
            return true;
        }
        case flipflop_jk: {
            require_equal(data.output_count, 2);

            // Q and !Q
            auto points = std::array {point_t {4, 0}, point_t {2, 2}};

            for (auto &&point : points) {
                if (!next_output(transform(data.position, data.orientation, point))) {
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
                        transform(data.position, data.orientation, point_t {width, y}))) {
                    return false;
                }
            }
            return true;
        }
    }
    throw_exception("'Don't know to calculate output locations.");
}

/// next_input(connection_id_t input_id, point_t position) -> bool;
template <typename Func>
auto iter_input_location_and_id(layout_calculation_data_t data, Func next_input) -> bool {
    return iter_input_location(
        data, [&, input_id = connection_id_t {0}](point_t position) mutable {
            return next_input(input_id++, position);
        });
}

/// next_output(connection_id_t output_id, point_t position) -> bool;
template <typename Func>
auto iter_output_location_and_id(layout_calculation_data_t data, Func next_output)
    -> bool {
    return iter_output_location(
        data, [&, output_id = connection_id_t {0}](point_t position) mutable {
            return next_output(output_id++, position);
        });
}

}  // namespace logicsim

#endif