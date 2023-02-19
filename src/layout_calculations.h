#ifndef LOGIKSIM_LAYOUT_CALCULATIONS_H
#define LOGIKSIM_LAYOUT_CALCULATIONS_H

#include "exceptions.h"
#include "layout.h"
#include "range.h"
#include "schematic.h"
#include "vocabulary.h"

#include <array>
#include <vector>

namespace logicsim {

[[nodiscard]] auto element_collision_rect(const Schematic &schematic,
                                          const Layout &layout, element_id_t element_id)
    -> rect_t;

auto require_min_input_count(Schematic::ConstElement element, std::size_t count) -> void;
auto require_min_output_count(Schematic::ConstElement element, std::size_t count) -> void;
auto require_input_count(Schematic::ConstElement element, std::size_t count) -> void;
auto require_output_count(Schematic::ConstElement element, std::size_t count) -> void;

namespace detail {
[[nodiscard]] auto transform(point_t element_position, DisplayOrientation orientation,
                             point_t offset) -> point_t;
}

/// next_point(point_t position) -> bool;
template <typename Func>
auto iter_element_body_points(const Schematic &schematic, const Layout &layout,
                              element_id_t element_id, Func next_point) -> bool {
    using detail::transform;
    const auto element = schematic.element(element_id);
    const auto orientation = layout.orientation(element_id);

    switch (element.element_type()) {
        using enum ElementType;

        case placeholder: {
            return true;
        }

        case wire: {
            return true;
        }

        case inverter_element: {
            return true;
        }

        case and_element:
        case or_element:
        case xor_element: {
            require_min_input_count(element, 2);
            const auto position = layout.position(element_id);

            const auto height = element.input_count();
            const auto output_offset = (height - element.output_count()) / 2;

            for (auto i : range(height)) {
                const auto y = grid_t {i};

                if (!next_point(transform(position, orientation, point_t {1, y}))) {
                    return false;
                }

                if (i != output_offset) {
                    if (!next_point(transform(position, orientation, point_t {2, y}))) {
                        return false;
                    }
                }
            }
            return true;
        }

        case clock_generator: {
            require_input_count(element, 2);
            const auto position = layout.position(element_id);

            using p = point_t;
            auto points = std::array {p {0, 0}, p {1, 0}, p {2, 0}, p {3, 0},
                                      p {0, 1}, p {0, 2}, p {1, 2}, p {3, 2}};

            for (auto &&point : points) {
                if (!next_point(transform(position, orientation, point))) {
                    return false;
                }
            }

            return true;
        }
        case flipflop_jk: {
            require_input_count(element, 5);
            const auto position = layout.position(element_id);

            using p = point_t;
            auto points = std::array {p {1, 0}, p {3, 0}, p {1, 1}, p {2, 1},
                                      p {3, 1}, p {4, 1}, p {1, 2}, p {3, 2}};

            for (auto &&point : points) {
                if (!next_point(transform(position, orientation, point))) {
                    return false;
                }
            }

            return true;
        }
        case shift_register: {
            require_min_input_count(element, 2);
            const auto position = layout.position(element_id);
            const auto output_count = element.output_count();

            // TODO width depends on internal state
            const auto width = 2 * 4;
            const auto height
                = output_count == 1
                      ? grid_t {1}
                      : grid_t {2 * (element.output_count() - std::size_t {1})};

            for (auto i : range(1, width)) {
                for (auto j : range(height + 1)) {
                    const auto x = grid_t {i};
                    const auto y = grid_t {j};
                    if (!next_point(transform(position, orientation, point_t {x, y}))) {
                        return false;
                    }
                }
            }

            for (auto j = 1; j < height; j += 2) {
                const auto x = grid_t {width};
                const auto y = grid_t {j};
                if (!next_point(transform(position, orientation, point_t {x, y}))) {
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
auto iter_input_location(const Schematic &schematic, const Layout &layout,
                         element_id_t element_id, Func next_input) -> bool {
    using detail::transform;
    const auto element = schematic.element(element_id);
    const auto orientation = layout.orientation(element_id);

    switch (element.element_type()) {
        using enum ElementType;

        case placeholder: {
            require_input_count(element, 1);
            return next_input(layout.position(element_id));
        }

        case wire: {
            require_input_count(element, 1);
            return next_input(layout.line_tree(element_id).input_position());
        }

        case inverter_element: {
            require_input_count(element, 1);
            return next_input(layout.position(element_id));
        }

        case and_element:
        case or_element:
        case xor_element: {
            require_min_input_count(element, 2);
            const auto position = layout.position(element_id);

            for (auto i : range(element.input_count())) {
                const auto y = grid_t {i};
                if (!next_input(transform(position, orientation, point_t {0, y}))) {
                    return false;
                }
            }
            return true;
        }

        case clock_generator: {
            require_input_count(element, 2);
            const auto position = layout.position(element_id);

            auto points = std::array {
                // internal input, placed such that it cannot be connected
                point_t {1, 1},
                // output signal
                point_t {2, 2},
            };

            for (auto &&point : points) {
                if (!next_input(transform(position, orientation, point))) {
                    return false;
                }
            }
            return true;
        }
        case flipflop_jk: {
            require_input_count(element, 5);
            const auto position = layout.position(element_id);

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
                if (!next_input(transform(position, orientation, point))) {
                    return false;
                }
            }
            return true;
        }
        case shift_register: {
            require_min_input_count(element, 2);
            const auto position = layout.position(element_id);

            // clock
            if (!next_input(transform(position, orientation, point_t {0, 1}))) {
                return false;
            }

            // memory row rows
            for (auto i : range(element.input_count() - std::size_t {1})) {
                const auto y = grid_t {2 * i};
                if (!next_input(transform(position, orientation, point_t {0, y}))) {
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
auto iter_output_location(const Schematic &schematic, const Layout &layout,
                          element_id_t element_id, Func next_output) -> bool {
    using detail::transform;
    const auto element = schematic.element(element_id);
    const auto orientation = layout.orientation(element_id);

    switch (element.element_type()) {
        using enum ElementType;

        case placeholder: {
            require_output_count(element, 0);
            return true;
        }

        case wire: {
            require_min_output_count(element, 1);
            const auto &line_tree = layout.line_tree(element_id);
            require_output_count(element, line_tree.output_count());

            for (auto &&point : line_tree.output_positions()) {
                next_output(point);
            }
            return true;
        }

        case inverter_element: {
            require_output_count(element, 1);
            const auto position = layout.position(element_id);
            next_output(transform(position, orientation, point_t {1, 0}));
            return true;
        }

        case and_element:
        case or_element:
        case xor_element: {
            require_output_count(element, 1);
            const auto position = layout.position(element_id);

            const auto height = element.input_count();
            const auto output_offset = grid_t {(height - element.output_count()) / 2};
            next_output(transform(position, orientation, point_t {2, output_offset}));
            return true;
        }

        case clock_generator: {
            require_output_count(element, 2);
            const auto position = layout.position(element_id);

            auto points = std::array {
                // internal input, placed such that it cannot be connected
                point_t {2, 1},
                // reset signal
                point_t {3, 1},
            };

            for (auto &&point : points) {
                if (!next_output(transform(position, orientation, point))) {
                    return false;
                }
            }
            return true;
        }
        case flipflop_jk: {
            require_output_count(element, 2);
            const auto position = layout.position(element_id);

            // Q and !Q
            auto points = std::array {point_t {4, 0}, point_t {2, 2}};

            for (auto &&point : points) {
                if (!next_output(transform(position, orientation, point))) {
                    return false;
                }
            }
            return true;
        }
        case shift_register: {
            require_min_output_count(element, 1);
            require_output_count(element, element.input_count() - std::size_t {1});
            const auto position = layout.position(element_id);

            // TODO width depends on internal state
            const auto width = 2 * 4;

            // for each memory row
            for (auto i : range(element.output_count())) {
                const auto y = grid_t {2 * i};
                if (!next_output(transform(position, orientation, point_t {width, y}))) {
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
auto iter_input_location_and_id(const Schematic &schematic, const Layout &layout,
                                element_id_t element_id, Func next_input) -> bool {
    return iter_input_location(
        schematic, layout, element_id,
        [&, input_id = connection_id_t {0}](point_t position) mutable {
            return next_input(input_id++, position);
        });
}

/// next_output(connection_id_t output_id, point_t position) -> bool;
template <typename Func>
auto iter_output_location_and_id(const Schematic &schematic, const Layout &layout,
                                 element_id_t element_id, Func next_output) -> bool {
    return iter_output_location(
        schematic, layout, element_id,
        [&, output_id = connection_id_t {0}](point_t position) mutable {
            return next_output(output_id++, position);
        });
}

}  // namespace logicsim

#endif