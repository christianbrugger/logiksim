#include "benchmark/simulation_runtime.h"

#include "algorithm/fmt_join.h"
#include "algorithm/uniform_int_distribution.h"
#include "component/simulation/history_view.h"
#include "logging.h"
#include "random/bool.h"
#include "schematic.h"
#include "schematic_generation.h"
#include "simulation.h"

namespace logicsim {

namespace {
void _generate_random_events(Rng &rng, Simulation &simulation) {
    const auto &schematic = simulation.schematic();

    const auto trigger_distribution = uint_distribution(0, 1);

    for (auto element_id : element_ids(schematic)) {
        for (auto input : inputs(schematic, element_id)) {
            if (!schematic.output(input) && trigger_distribution(rng) == 0) {
                simulation.set_unconnected_input(input, !simulation.input_value(input));
            }
        }
    }
}

}  // namespace

auto benchmark_simulation(Rng &rng, Schematic &&schematic, const int n_events,
                          const PrintEvents do_print) -> int64_t {
    Simulation simulation {std::move(schematic), do_print};

    while (true) {
        simulation.run(simulation::defaults::infinite_simulation_time,
                       simulation::defaults::no_realtime_timeout,
                       n_events - simulation.processed_event_count());

        if (simulation.processed_event_count() >= n_events) {
            break;
        }

        _generate_random_events(rng, simulation);
    }

    if (do_print == PrintEvents::yes) {
        // TODO move to simulation.format()

        // auto output_values {simulation.output_values()};

        print_fmt("events simulated = {}\n", simulation.processed_event_count());
        // print_fmt("input_values = {}\n", fmt_join("", simulation.input_values(),
        // "{:b}")); print_fmt("output_values = {}\n", fmt_join("", output_values,
        // "{:b}"));
        for (auto element_id : element_ids(schematic)) {
            if (schematic.element_type(element_id) == ElementType::wire) {
                const auto history = simulation.input_history(element_id);
                print(element_id, history);
            }
        }
    }

    Ensures(simulation.processed_event_count() >= n_events);
    return simulation.processed_event_count();
}

auto benchmark_simulation(const int n_elements, const int m_events,
                          const PrintEvents do_print) -> int64_t {
    auto rng = Rng {0};

    auto schematic = create_random_schematic(rng, n_elements);
    schematic = with_custom_delays(rng, schematic);

    if (do_print == PrintEvents::yes) {
        print(schematic);
    }
    add_missing_placeholders(schematic);

    return benchmark_simulation(rng, std::move(schematic), m_events, do_print);
}

auto benchmark_simulation_metastable(Schematic &&schematic, const int n_events,
                                     const PrintEvents do_print) -> int64_t {
    Simulation simulation {std::move(schematic), do_print};

    while (true) {
        // we use realtime timeout, to see the impact of its checking
        simulation.run(simulation::defaults::infinite_simulation_time,
                       simulation::realtime_timeout_t {1000 * 1ms},
                       n_events - simulation.processed_event_count());

        if (simulation.is_finished()) {
            break;
        }
        if (simulation.processed_event_count() >= n_events) {
            break;
        }
    }

    if (do_print == PrintEvents::yes) {
        // TODO move to simulation.format()

        // auto output_values {simulation.output_values()};

        print_fmt("events simulated = {}\n", simulation.processed_event_count());
        // print_fmt("input_values = {}\n", fmt_join("", simulation.input_values(),
        // "{:b}")); print_fmt("output_values = {}\n", fmt_join("", output_values,
        // "{:b}"));
        for (auto element_id : element_ids(schematic)) {
            if (schematic.element_type(element_id) == ElementType::wire) {
                const auto history = simulation.input_history(element_id);
                print(element_id, history);
            }
        }
    }

    return simulation.processed_event_count();
}

}  // namespace logicsim
