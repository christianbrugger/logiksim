#ifndef LOGICSIM_BENCHMARK_SIMULATION_RUNTIME_H
#define LOGICSIM_BENCHMARK_SIMULATION_RUNTIME_H

#include "benchmark/schematic_creation.h"
#include "random/generator.h"
#include "vocabulary/print_events.h"

namespace logicsim {

class SchematicOld;

constexpr int BENCHMARK_DEFAULT_EVENTS {10'000};

auto benchmark_simulation(Rng &rng, SchematicOld &schematic, int n_events,
                          PrintEvents do_print = PrintEvents::no) -> int64_t;

auto benchmark_simulation(int n_elements = BENCHMARK_DEFAULT_ELEMENTS,
                          int n_events = BENCHMARK_DEFAULT_EVENTS,
                          PrintEvents do_print = PrintEvents::no) -> int64_t;

auto benchmark_simulation_pure(SchematicOld &schematic, int n_events,
                               PrintEvents do_print = PrintEvents::no) -> int64_t;

}  // namespace logicsim

#endif
