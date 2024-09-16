
#include "algorithm/range.h"
#include "benchmark/render_line_scene.h"
#include "circuit_example.h"
#include "default_element_definition.h"
#include "editable_circuit.h"
#include "format/blend2d_type.h"
#include "geometry/scene.h"
#include "logging.h"
#include "render/circuit/render_circuit.h"
#include "render/image_surface.h"
#include "timer.h"
#include "vocabulary/widget_render_config.h"

#include <fmt/core.h>
#include <gsl/gsl>

#include <exception>
#include <iostream>

namespace logicsim {

auto test_render() -> void {
    auto cache = cache_with_default_fonts();

    auto editable_circuit = load_example_with_logging(2);
    visible_selection_select_all(editable_circuit);

    for (auto _ [[maybe_unused]] : range(3)) {
        auto timer = Timer {"Example Circuit Render", Timer::Unit::ms, 3};
        render_layout_to_file(editable_circuit.layout(),
                              editable_circuit.visible_selection(), "test_circuit.png",
                              create_context_render_settings(BLSizeI {800, 600}), cache);
    }
}

}  // namespace logicsim

auto main() -> int {
    using namespace logicsim;

    bool do_run = true;

    /// TODO consider: ios_base::sync_with_stdio(false);
    /// SL.io.10 in https://isocpp.github.io/CppCoreGuidelines/

    test_render();

    if (do_run) {
        try {
            auto timer = Timer {"Benchmark + Render", Timer::Unit::ms, 3};
            // auto count = benchmark_simulation(6, 10, true);
            // auto count = logicsim::benchmark_simulation(BENCHMARK_DEFAULT_ELEMENTS,
            //                                            BENCHMARK_DEFAULT_EVENTS, true);

            auto count = logicsim::benchmark_line_renderer(100, true);
            print_fmt("count = {}\n", count);

        } catch (const std::exception& exc) {
            std::cerr << exc.what() << '\n';
            return -1;
        }
    }

    return 0;
}
