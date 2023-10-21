#include "benchmark/simulation_runtime.h"

#include "algorithm/fmt_join.h"
#include "algorithm/uniform_int_distribution.h"
#include "component/simulation/history_iterator.h"
#include "component/simulation/history_view.h"
#include "logging.h"
#include "random/bool.h"
#include "schematic_validation.h"

namespace logicsim {

namespace {

void _generate_random_events(Rng &rng, Simulation &simulation) {
    const auto trigger_distribution = uint_distribution(0, 1);

    for (auto element : simulation.schematic().elements()) {
        for (auto input : element.inputs()) {
            if (trigger_distribution(rng) == 0) {
                simulation.submit_event(input, delay_t {1us},
                                        !simulation.input_value(input));
            }
        }
    }
}

}  // namespace

auto benchmark_simulation(Rng &rng, SchematicOld &schematic, const int n_events,
                          const bool do_print) -> int64_t {
    Simulation simulation {schematic};
    simulation.print_events = do_print;

    // set custom delays
    for (const auto element : schematic.elements()) {
        for (const auto output : element.outputs()) {
            auto delay_dist = uint_distribution<delay_t::rep>(5, 500);
            output.set_delay(delay_t {1us * delay_dist(rng)});
        }
    }

    // set history for wires
    for (const auto element : schematic.elements()) {
        if (element.element_type() == ElementType::wire) {
            const auto delay = element.output(connection_id_t {0}).delay();
            element.set_history_length(delay * 10);
        }
    }

    simulation.initialize();

    int64_t simulated_event_count {0};
    while (true) {
        simulated_event_count += simulation.run(
            simulation::defaults::infinite_simulation_time,
            simulation::defaults::no_timeout, n_events - simulated_event_count);

        if (simulated_event_count >= n_events) {
            break;
        }

        _generate_random_events(rng, simulation);
    }

    if (do_print) {
        // auto output_values {simulation.output_values()};

        print_fmt("events simulated = {}\n", simulated_event_count);
        // print_fmt("input_values = {}\n", fmt_join("", simulation.input_values(),
        // "{:b}")); print_fmt("output_values = {}\n", fmt_join("", output_values,
        // "{:b}"));
        for (auto element : schematic.elements()) {
            if (element.element_type() == ElementType::wire) {
                auto hist =
                    simulation.input_history(SchematicOld::ConstElement {element});
                print(element, hist);
            }
        }
    }

    Ensures(simulated_event_count >= n_events);
    return simulated_event_count;
}

auto benchmark_simulation(const int n_elements, const int n_events, const bool do_print)
    -> int64_t {
    auto rng = Rng {0};

    auto schematic = create_random_schematic(rng, n_elements);
    if (do_print) {
        print(schematic);
    }
    add_output_placeholders(schematic);
    validate(schematic, schematic::validate_all);

    return benchmark_simulation(rng, schematic, n_events, do_print);
}

auto benchmark_simulation_pure(SchematicOld &schematic, const int n_events,
                               const bool do_print) -> int64_t {
    Simulation simulation {schematic};
    simulation.print_events = do_print;

    set_default_inputs(simulation);
    set_default_outputs(simulation);
    simulation.initialize();

    int64_t simulated_event_count {0};
    while (true) {
        const auto event_count = simulation.run(
            simulation::defaults::infinite_simulation_time,
            simulation::timeout_t {1000 * 1ms}, n_events - simulated_event_count);
        // const auto event_count = simulation.run(
        //     Simulation::defaults::infinite_simulation_time,
        //     Simulation::defaults::no_timeout, n_events - simulated_event_count);
        simulated_event_count += event_count;

        if (event_count == 0) {
            break;
        }
        if (simulated_event_count >= n_events) {
            break;
        }
    }

    if (do_print) {
        // auto output_values {simulation.output_values()};

        print_fmt("events simulated = {}\n", simulated_event_count);
        // print_fmt("input_values = {}\n", fmt_join("", simulation.input_values(),
        // "{:b}")); print_fmt("output_values = {}\n", fmt_join("", output_values,
        // "{:b}"));
        for (auto element : schematic.elements()) {
            if (element.element_type() == ElementType::wire) {
                auto hist =
                    simulation.input_history(SchematicOld::ConstElement {element});
                print(element, hist);
            }
        }
    }

    return simulated_event_count;
}

}  // namespace logicsim
