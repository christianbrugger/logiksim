#ifndef LOGIKSIM_LAYOUT_CALCULATIONS_H
#define LOGIKSIM_LAYOUT_CALCULATIONS_H

#include "exceptions.h"
#include "layout.h"
#include "range.h"
#include "schematic.h"
#include "vocabulary.h"

#include <vector>

namespace logicsim {

[[nodiscard]] auto element_collision_body(const Schematic &schematic,
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

template <typename Func>
auto for_each_input_location(const Schematic &schematic, const Layout &layout,
                             element_id_t element_id, Func next_input) -> void {
    using detail::transform;
    const auto element = schematic.element(element_id);
    const auto orientation = layout.orientation(element_id);

    switch (element.element_type()) {
        using enum ElementType;

        case placeholder: {
            require_input_count(element, 1);
            next_input(layout.position(element_id));
            return;
        }

        case wire: {
            require_input_count(element, 1);
            next_input(layout.line_tree(element_id).input_position());
            return;
        }

        case inverter_element: {
            require_input_count(element, 1);
            next_input(layout.position(element_id));
            return;
        }

        case and_element:
        case or_element:
        case xor_element: {
            require_min_input_count(element, 2);
            const auto position = layout.position(element_id);

            for (auto i : range(element.input_count())) {
                const auto y = grid_t {i};
                next_input(transform(position, orientation, point_t {0, y}));
            }
            return;
        }

        case clock_generator: {
            require_input_count(element, 2);
            const auto position = layout.position(element_id);

            // internal input, not connectable
            next_input(transform(position, orientation, point_t {1, 1}));
            // reset signal
            next_input(transform(position, orientation, point_t {2, 2}));

            return;
        }
        case flipflop_jk: {
            require_input_count(element, 5);
            const auto position = layout.position(element_id);

            // clock
            next_input(transform(position, orientation, point_t {0, 1}));
            // j & k
            next_input(transform(position, orientation, point_t {0, 0}));
            next_input(transform(position, orientation, point_t {0, 2}));
            // set & reset
            next_input(transform(position, orientation, point_t {2, 0}));
            next_input(transform(position, orientation, point_t {2, 2}));

            return;
        }
        case shift_register: {
            require_min_input_count(element, 2);
            const auto position = layout.position(element_id);

            // clock
            next_input(transform(position, orientation, point_t {0, 1}));
            // for each memory row
            for (auto i : range(element.input_count() - std::size_t {1})) {
                const auto y = grid_t {2 * i};
                next_input(transform(position, orientation, point_t {0, y}));
            }
            return;
        }
    }

    throw_exception("'Don't know to calculate input locations.");
}

template <typename Func>
auto for_each_output_location(const Schematic &schematic, const Layout &layout,
                              element_id_t element_id, Func next_output) -> void {
    using detail::transform;
    const auto element = schematic.element(element_id);
    const auto orientation = layout.orientation(element_id);

    switch (element.element_type()) {
        using enum ElementType;

        case placeholder: {
            require_output_count(element, 0);
            return;
        }

        case wire: {
            require_min_output_count(element, 1);
            const auto &line_tree = layout.line_tree(element_id);
            require_output_count(element, line_tree.output_count());

            for (auto &&point : line_tree.output_positions()) {
                next_output(point);
            }
            return;
        }

        case inverter_element: {
            require_output_count(element, 1);
            const auto position = layout.position(element_id);
            next_output(transform(position, orientation, point_t {1, 0}));
            return;
        }

        case and_element:
        case or_element:
        case xor_element: {
            require_output_count(element, 1);
            const auto position = layout.position(element_id);

            const auto height = element.input_count();
            const auto output_offset = grid_t {(height - element.output_count()) / 2};
            next_output(transform(position, orientation, point_t {2, output_offset}));
            return;
        }

        case clock_generator: {
            require_output_count(element, 2);
            const auto position = layout.position(element_id);

            // internal output, not connectable
            next_output(transform(position, orientation, point_t {2, 1}));
            // reset signal
            next_output(transform(position, orientation, point_t {3, 1}));
            return;
        }
        case flipflop_jk: {
            require_output_count(element, 2);
            const auto position = layout.position(element_id);

            // Q and !Q
            next_output(transform(position, orientation, point_t {4, 0}));
            next_output(transform(position, orientation, point_t {4, 2}));
            return;
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
                next_output(transform(position, orientation, point_t {width, y}));
            }
            return;
        }
    }
    throw_exception("'Don't know to calculate output locations.");
}

/// next_input(connection_id_t input_id, point_t position) -> void;
template <typename Func>
auto for_each_input_location_and_id(const Schematic &schematic, const Layout &layout,
                                    element_id_t element_id, Func next_input) -> void {
    for_each_input_location(
        schematic, layout, element_id,
        [&, input_id = connection_id_t {0}](point_t position) mutable {
            next_input(input_id, position);
            ++input_id;
        });
}

/// next_output(connection_id_t output_id, point_t position) -> void;
template <typename Func>
auto for_each_output_location_and_id(const Schematic &schematic, const Layout &layout,
                                     element_id_t element_id, Func next_output) -> void {
    for_each_output_location(
        schematic, layout, element_id,
        [&, output_id = connection_id_t {0}](point_t position) mutable {
            next_output(output_id, position);
            ++output_id;
        });
}

}  // namespace logicsim

#endif