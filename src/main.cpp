
#include "algorithm/fmt_join.h"
#include "benchmark/render_line_scene.h"
#include "editable_circuit/cache/connection_cache.h"
#include "editable_circuit/cache/helper.h"
#include "editable_circuit/editable_circuit.h"
#include "layout_calculation.h"
#include "logging.h"
#include "render_circuit.h"
#include "simulation.h"
#include "timer.h"

#include <fmt/core.h>
#include <gsl/gsl>

#include <QCoreApplication>

#include <exception>
#include <iostream>

namespace logicsim {

//

}  // namespace logicsim

auto main(int argc, char** argv) -> int {
    using namespace logicsim;
    const auto app [[maybe_unused]] = QCoreApplication {argc, argv};

    bool do_run = true;

    /// TODO consider: ios_base::sync_with_stdio(false);
    /// SL.io.10 in https://isocpp.github.io/CppCoreGuidelines/

    if (do_run) {
        try {
            auto timer = Timer {"Benchmark", Timer::Unit::ms, 3};
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
