#include "interactive_simulation.h"

#include "algorithm/round.h"
#include "format/container.h"
#include "format/std_type.h"
#include "layout.h"
#include "schematic.h"
#include "schematic_generation.h"
#include "vocabulary/internal_state.h"
#include "vocabulary/simulation_setting.h"

#include <stdexcept>

namespace logicsim {

namespace detail::interactive_simulation {

auto iteraction_data_t::format() const -> std::string {
    return fmt::format("{}", element_id);
}

InteractionCache::InteractionCache(const Layout& layout) {
    for (auto element : layout.elements()) {
        if (element.element_type() == ElementType::button) {
            auto& data = map_[element.position()];

            if (data.element_id != null_element) [[unlikely]] {
                throw std::runtime_error("map entry is not empty");
            }

            data.element_id = element;
        }
    }
}

auto InteractionCache::format() const -> std::string {
    return fmt::format("<InteractionCache: {}>", map_);
}

auto InteractionCache::find(point_t position) const -> std::optional<element_id_t> {
    if (const auto it = map_.find(position); it != map_.end()) {
        return it->second.element_id;
    }
    return std::nullopt;
}

}  // namespace detail::interactive_simulation

//
// Interactive Simulation
//

InteractiveSimulation::InteractiveSimulation(const Layout& layout,
                                             const SimulationSettings& settings)
    : simulation_ {generate_schematic(layout, settings.wire_delay_per_distance())},
      last_event_count_ {simulation_.processed_event_count()},
      interaction_cache_ {layout},
      simulation_time_rate_ {settings.simulation_time_rate},
      wire_delay_per_distance_ {settings.wire_delay_per_distance()},

      simulation_time_reference_ {simulation_.time()},
      realtime_reference_ {timer_t::now()} {}

auto InteractiveSimulation::schematic() const -> const Schematic& {
    return simulation_.schematic();
}

auto InteractiveSimulation::simulation() const -> const Simulation& {
    return simulation_;
}

auto InteractiveSimulation::set_simulation_time_rate(time_rate_t time_rate) -> void {
    if (time_rate < time_rate_t {0us}) [[unlikely]] {
        throw std::runtime_error("time rate cannot be negative");
    }

    const auto realtime_now = timer_t::now();
    simulation_time_reference_ = expected_simulation_time(realtime_now);
    realtime_reference_ = realtime_now;

    simulation_time_rate_ = time_rate;
}

auto InteractiveSimulation::time_rate() const -> time_rate_t {
    return simulation_time_rate_;
}

auto InteractiveSimulation::time() const -> time_t {
    return simulation_.time();
}

auto InteractiveSimulation::wire_delay_per_distance() const -> delay_t {
    return wire_delay_per_distance_;
}

auto InteractiveSimulation::run(simulation::realtime_timeout_t timeout) -> void {
    const auto start_realtime = timer_t::now();
    const auto start_simulation_time = simulation_.time();

    const auto expected_time = expected_simulation_time(start_realtime);
    const auto time_to_simulate = expected_time - start_simulation_time;

    if (time_to_simulate <= delay_t {0us}) {
        return;
    }

    simulation_.run(time_to_simulate, timeout);

    {
        const auto event_count = simulation_.processed_event_count();
        event_counter_.count_events(event_count - last_event_count_);
        last_event_count_ = event_count;
    }

    // in case simulation is too slow, allow us to catch up
    if (expected_time > simulation_.time()) {
        realtime_reference_ = start_realtime;
        simulation_time_reference_ = start_simulation_time;
    }
}

auto InteractiveSimulation::is_finished() const -> bool {
    return simulation_.is_finished();
}

auto InteractiveSimulation::mouse_press(point_t position) -> void {
    const auto element_id = interaction_cache_.find(position);

    if (element_id) {
        const auto state = internal_state_t {*element_id, internal_state_index_t {0}};

        const auto value = simulation_.internal_state(state);
        simulation_.set_internal_state(state, !value);
    }
}

auto InteractiveSimulation::events_per_second() const -> double {
    return event_counter_.events_per_second();
}

auto InteractiveSimulation::expected_simulation_time(realtime_t now) const -> time_t {
    const auto realtime_delta = std::chrono::duration<double> {now - realtime_reference_};
    const auto time_delta_ns = realtime_delta / std::chrono::seconds {1} *
                               simulation_time_rate_.rate_per_second.count_ns();

    const auto time_delta = delay_t {round_to<delay_t::rep>(time_delta_ns) * 1ns};
    return time_t {simulation_time_reference_ + time_delta};
}

}  // namespace logicsim
