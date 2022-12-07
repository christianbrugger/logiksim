
#include "circuit.h"
#include "simulation.h"

#include <exception>
#include <iostream>

auto main() -> int {
    using namespace logicsim;

    try {
        benchmark_simulation(100, true);
        // Circuit circuit;
        // auto test = circuit.element(0);
        // test.element_id();

    } catch (const std::exception& exc) {
        std::cerr << exc.what() << '\n';
        return -1;
    }
    return 0;
}
