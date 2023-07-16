#include "interactive_simulation.h"

#include "schematic_generation.h"

namespace logicsim {

namespace detail::interactive_simulation {

auto iteraction_data_t::format() const -> std::string {
    return fmt::format("<{}>", element_id);
}

InteractionCache::InteractionCache(const Layout& layout) {
    for (auto element : layout.elements()) {
        if (element.element_type() == ElementType::button) {
            auto& data = map_[element.position()];

            if (data.element_id != null_element) [[unlikely]] {
                throw_exception("map entry is not empty");
            }

            data.element_id = element;
        }
    }
}

}  // namespace detail::interactive_simulation

InteractiveSimulation::InteractiveSimulation(const Layout& layout)
    : schematic_ {generate_schematic(layout)},
      simulation_ {schematic_},
      interaction_cache_ {layout},

      simulation_time_reference_ {simulation_.time()},
      realtime_reference_ {timer_t::now()} {
    // TODO remove when not needed anymore
    for (auto element : schematic_.elements()) {
        if (element.output_count() > 0) {
            simulation_.set_output_delays(element, element.output_delays());
            simulation_.set_history_length(element, element.history_length());
        }
    }
    for (auto element : schematic_.elements()) {
        if (element.element_type() == ElementType::button) {
            simulation_.set_internal_state(element, 0, true);
        }
    }

    // keep this
    simulation_.initialize();
}

auto InteractiveSimulation::schematic() const -> const Schematic& {
    return schematic_;
}

auto InteractiveSimulation::simulation() const -> const Simulation& {
    return simulation_;
}

auto InteractiveSimulation::set_time_rate(time_rate_t time_rate) -> void {
    if (time_rate < time_rate_t {0us}) [[unlikely]] {
        throw_exception("time rate cannot be negative");
    }

    simulation_time_reference_ = expected_simulation_time();
    realtime_reference_ = timer_t::now();

    time_rate_ = time_rate;
}

auto InteractiveSimulation::time_rate() const -> time_rate_t {
    return time_rate_;
}

auto InteractiveSimulation::time() const -> time_t {
    return simulation_.time();
}

auto InteractiveSimulation::run(timeout_t timeout) -> int64_t {
    const auto time_to_simulate
        = expected_simulation_time().value - simulation_.time().value;

    if (time_to_simulate > 0us) {
        return simulation_.run(time_to_simulate, timeout);
    }
    return 0;
}

auto InteractiveSimulation::expected_simulation_time() const -> time_t {
    const auto realtime_delta
        = std::chrono::duration<double> {timer_t::now() - realtime_reference_};
    const auto time_delta_double
        = realtime_delta / 1000ms * time_rate_.rate_per_second.value;

    const auto time_delta_int
        = std::chrono::duration_cast<time_t::value_type>(time_delta_double);
    return time_t {simulation_time_reference_.value + time_delta_int};
}

}  // namespace logicsim
