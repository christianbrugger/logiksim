
#include "editable_circuit/caches/connection_cache.h"
#include "editable_circuit/caches/helpers.h"
#include "editable_circuit/editable_circuit.h"
#include "layout_calculations.h"
#include "renderer.h"
#include "simulation.h"
#include "timer.h"

#include <fmt/core.h>

#include <exception>
#include <iostream>

namespace logicsim {

///////////

//////////

auto generate_layout() -> Layout {
    constexpr auto max_value = 50;
    auto editable_circuit = EditableCircuit {Layout {}};

    for (auto x : range(5, max_value, 5)) {
        for (auto y : range(5, max_value, 5)) {
            const auto definition = LogicItemDefinition {
                .element_type = ElementType::or_element,
                .input_count = 3,
                .output_count = 1,
            };

            editable_circuit.add_logic_item(definition, point_t {grid_t {x}, grid_t {y}},
                                            InsertionMode::insert_or_discard);

            editable_circuit.add_line_segments(point_t {grid_t {x + 2}, grid_t {y + 1}},
                                               point_t {grid_t {x + 4}, grid_t {y - 1}},
                                               LineInsertionType::horizontal_first,
                                               InsertionMode::insert_or_discard);

            editable_circuit.add_line_segments(point_t {grid_t {x + 3}, grid_t {y + 1}},
                                               point_t {grid_t {x + 5}, grid_t {y + 2}},
                                               LineInsertionType::vertical_first,
                                               InsertionMode::insert_or_discard);
        }
    }

    return editable_circuit.extract_layout();
}

auto benchmark_layered_drawing() -> void {
    const auto layout = generate_layout();

    print(layout.format_stats());
}

}  // namespace logicsim

auto main() -> int {
    using namespace logicsim;

    /// TODO consider: ios_base::sync_with_stdio(false);
    /// SL.io.10 in https://isocpp.github.io/CppCoreGuidelines/

    benchmark_layered_drawing();

    try {
        auto timer = Timer {"Benchmark", Timer::Unit::ms, 3};
        // auto count = benchmark_simulation(6, 10, true);
        // auto count = logicsim::benchmark_simulation(BENCHMARK_DEFAULT_ELEMENTS,
        //                                            BENCHMARK_DEFAULT_EVENTS, true);

        auto count = logicsim::benchmark_line_renderer(100, true);
        fmt::print("count = {}\n", count);

    } catch (const std::exception& exc) {
        std::cerr << exc.what() << '\n';
        return -1;
    }

    return 0;
}
