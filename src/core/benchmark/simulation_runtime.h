#ifndef LOGICSIM_BENCHMARK_SIMULATION_RUNTIME_H
#define LOGICSIM_BENCHMARK_SIMULATION_RUNTIME_H

#include "core/random/generator.h"
#include "core/random/random_schematic.h"
#include "core/vocabulary/print_events.h"

namespace logicsim {

class Schematic;

namespace simulation_benchmark::defaults {
constexpr inline auto event_count = 10'000;
}  // namespace simulation_benchmark::defaults

/**
 * @brief: Generates random input events and simulates the circuit until given events.
 *
 * If the simulation reaches a steady state more input events are generated until
 * the total number of events are simulated.
 *
 * Returns the exact number of simulated events.
 */
auto benchmark_simulation(Rng &rng, Schematic &&schematic,
                          int n_events = simulation_benchmark::defaults::event_count,
                          PrintEvents do_print = PrintEvents::no) -> int64_t;

/**
 * @brief: Generate a random with n elements and simulate m events.
 *
 * If the simulation reaches a steady state more input events are generated until
 * the total number of events are simulated.
 *
 * Returns the exact number of simulated events.
 */
auto benchmark_simulation(int n_elements = random::defaults::schematic_element_count,
                          int m_events = simulation_benchmark::defaults::event_count,
                          PrintEvents do_print = PrintEvents::no) -> int64_t;

/**
 * @brief: Runs the simulation for until at least n_events are simulated or it stops.
 *
 * Note that the method returns if a steady state is reached. This benchmark
 * is therefore only useful with recursive circuits and with metastability.
 *
 * Returns the exact number of simulated events.
 */
auto benchmark_simulation_metastable(
    Schematic &&schematic, int n_events = simulation_benchmark::defaults::event_count,
    PrintEvents do_print = PrintEvents::no) -> int64_t;

}  // namespace logicsim

#endif
