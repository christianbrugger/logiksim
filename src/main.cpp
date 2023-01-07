
#include "simulation.h"
#include "timer.h"

#include <fmt/core.h>

#include <exception>
#include <iostream>

auto main() -> int {
    using namespace logicsim;

    /// TODO consider: ios_base::sync_with_stdio(false);
    /// SL.io.10 in https://isocpp.github.io/CppCoreGuidelines/

    try {
        auto timer = Timer {"Benchmark", Timer::Unit::ms, 3};
        // auto count = benchmark_simulation(6, 10, true);
        auto count = logicsim::benchmark_simulation(BENCHMARK_DEFAULT_ELEMENTS,
                                                    BENCHMARK_DEFAULT_EVENTS, true);
        fmt::print("count = {}\n", count);

    } catch (const std::exception& exc) {
        std::cerr << exc.what() << '\n';
        return -1;
    }
    return 0;
}
