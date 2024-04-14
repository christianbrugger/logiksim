#include "circuit_example.h"

#include "algorithm/range_step.h"
#include "editable_circuit.h"
#include "logging.h"
#include "random/generator.h"
#include "timer.h"
#include "vocabulary/grid.h"
#include "vocabulary/insertion_mode.h"
#include "vocabulary/point.h"

#include <stdexcept>

namespace logicsim {

namespace {

constexpr inline auto max_grid_debug = 50;
constexpr inline auto max_grid_release = 1600;  //  1600;

#ifdef NDEBUG
constexpr inline auto is_debug_build = false;
#else
constexpr inline auto is_debug_build = true;
#endif

constexpr inline auto max_grid_value = is_debug_build ? max_grid_debug : max_grid_release;

auto load_circuit_example_1(EditableCircuit& editable_circuit) -> void {
    auto rng = get_random_number_generator();
    add_example(rng, editable_circuit);
}

auto load_circuit_example_2(EditableCircuit& editable_circuit) -> void {
    const auto definition = LogicItemDefinition {
        .logicitem_type = LogicItemType::or_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .orientation = orientation_t::right,
        .output_inverters = {true},
    };

    for (auto x : range(5, max_grid_value, 5)) {
        for (auto y : range(5, max_grid_value, 5)) {
            editable_circuit.add_logicitem(definition, point_t {grid_t {x}, grid_t {y}},
                                           InsertionMode::insert_or_discard);

            add_wire_segments(editable_circuit, point_t {grid_t {x + 2}, grid_t {y + 1}},
                              point_t {grid_t {x + 4}, grid_t {y - 1}},
                              LineInsertionType::horizontal_first,
                              InsertionMode::insert_or_discard);

            add_wire_segments(editable_circuit, point_t {grid_t {x + 3}, grid_t {y + 1}},
                              point_t {grid_t {x + 5}, grid_t {y + 2}},
                              LineInsertionType::vertical_first,
                              InsertionMode::insert_or_discard);
        }
    }
}

auto load_circuit_example_3(EditableCircuit& editable_circuit) -> void {
    const auto definition = LogicItemDefinition {
        .logicitem_type = LogicItemType::or_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .orientation = orientation_t::right,
        .output_inverters = {true},
    };

    for (auto x : range(5, max_grid_value, 5)) {
        for (auto y : range(5, max_grid_value, 5)) {
            editable_circuit.add_logicitem(definition, point_t {grid_t {x}, grid_t {y}},
                                           InsertionMode::insert_or_discard);
        }
    }
}

auto load_circuit_example_4(EditableCircuit& editable_circuit) -> void {
    for (auto x : range(5, max_grid_value, 5)) {
        for (auto y : range(5, max_grid_value, 5)) {
            add_wire_segments(editable_circuit, point_t {grid_t {x + 2}, grid_t {y + 1}},
                              point_t {grid_t {x + 4}, grid_t {y - 1}},
                              LineInsertionType::horizontal_first,
                              InsertionMode::insert_or_discard);

            add_wire_segments(editable_circuit, point_t {grid_t {x + 3}, grid_t {y + 1}},
                              point_t {grid_t {x + 5}, grid_t {y + 2}},
                              LineInsertionType::vertical_first,
                              InsertionMode::insert_or_discard);
        }
    }
}

auto load_circuit_example(EditableCircuit& editable_circuit, int number) -> void {
    switch (number) {
        case 1: {
            return load_circuit_example_1(editable_circuit);
        }
        case 2: {
            return load_circuit_example_2(editable_circuit);
        }
        case 3: {
            return load_circuit_example_3(editable_circuit);
        }
        case 4: {
            return load_circuit_example_4(editable_circuit);
        }

        default: {
            throw std::runtime_error("unknown circuit example number");
        }
    }
}

}  // namespace

auto load_example_with_logging(int number) -> EditableCircuit {
    auto timer = Timer {"", Timer::Unit::ms, 1};

    auto editable_circuit = EditableCircuit {};
    load_circuit_example(editable_circuit, number);

    // count & print
    {
        const auto timer_str = timer.format();
        const auto& layout = editable_circuit.layout();

        auto logicitem_count = layout.logic_items().size();
        auto segment_count = get_segment_count(layout);

        if (layout.size() < 10) {
            print(editable_circuit);
        }
        print_fmt("Added {} elements and {} wire segments in {}.\n", logicitem_count,
                  segment_count, timer_str);
    }

    return editable_circuit;
}

}  // namespace logicsim
