
#include "circuit.h"
#include "simulation.h"

#include <exception>
#include <iostream>

auto main() -> int {
    try {
        logicsim::benchmark_simulation(100, true);
    } catch (const std::exception& exc) {
        std::cerr << exc.what() << '\n';
        return -1;
    }
    return 0;
}
