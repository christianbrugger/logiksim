#ifndef LOGICSIM_BENCHMARK_SIMULATION_RUNTIME_H
#define LOGICSIM_BENCHMARK_SIMULATION_RUNTIME_H

#include "benchmark/schematic_creation.h"
#include "simulation.h"

namespace logicsim {

constexpr int BENCHMARK_DEFAULT_EVENTS {10'000};

template <std::uniform_random_bit_generator G>
auto benchmark_simulation(G &rng, Schematic &schematic, const int n_events,
                          const bool do_print) -> int64_t;
auto benchmark_simulation(int n_elements = BENCHMARK_DEFAULT_ELEMENTS,
                          int n_events = BENCHMARK_DEFAULT_EVENTS, bool do_print = false)
    -> int64_t;

auto benchmark_simulation_pure(Schematic &schematic, const int n_events,
                               const bool do_print) -> int64_t;

}  // namespace logicsim

#endif
