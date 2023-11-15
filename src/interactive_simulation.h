#ifndef LOGIKSIM_INTERACTIVE_SIMULATION_H
#define LOGIKSIM_INTERACTIVE_SIMULATION_H

#include "component/interactive_simulation/interaction_cache.h"
#include "event_counter_multi.h"
#include "spatial_simulation.h"
#include "timer.h"
#include "vocabulary/time.h"
#include "vocabulary/time_rate.h"

#include <chrono>

namespace logicsim {

/**
 * @brief: Simulation that support mouse based interaction and can be run at
 *         a defined pace.
 *
 * Class-invariants:
 *     + realtime_reference_ <= timer_t::now()
 *     + last_event_count_ <= simulation.processed_event_count()
 *     + simulation_time_rate_ >= time_rate_t {0us}
 */
class InteractiveSimulation {
    using timer_t = std::chrono::steady_clock;
    using realtime_t = timer_t::time_point;

   public:
    struct defaults {
        constexpr static auto standard_timeout = realtime_timeout_t {1ms};
    };

   public:
    [[nodiscard]] explicit InteractiveSimulation();
    [[nodiscard]] explicit InteractiveSimulation(SpatialSimulation&& spatial_simulation,
                          time_rate_t simulation_time_rate);
    [[nodiscard]] explicit InteractiveSimulation(Layout&& layout,
                                                 delay_t wire_delay_per_distance,
                          time_rate_t simulation_time_rate);

    [[nodiscard]] auto spatial_simulation() const -> const SpatialSimulation&;
    [[nodiscard]] auto layout() const -> const Layout&;
    [[nodiscard]] auto schematic() const -> const Schematic&;
    [[nodiscard]] auto simulation() const -> const Simulation&;

    auto set_simulation_time_rate(time_rate_t time_rate) -> void;
    [[nodiscard]] auto time_rate() const -> time_rate_t;
    [[nodiscard]] auto time() const -> time_t;
    [[nodiscard]] auto wire_delay_per_distance() const -> delay_t;
    [[nodiscard]] auto events_per_second() const -> double;

    auto run(realtime_timeout_t timeout = defaults::standard_timeout) -> void;
    [[nodiscard]] auto is_finished() const -> bool;
    auto mouse_press(point_t position) -> void;

   private:
    [[nodiscard]] auto expected_simulation_time(realtime_t now) const -> time_t;

   private:
    SpatialSimulation spatial_simulation_;
    interactive_simulation::InteractionCache interaction_cache_;

    time_rate_t simulation_time_rate_;
    realtime_t realtime_reference_;
    time_t simulation_time_reference_;

    Simulation::event_count_t last_event_count_;
    MultiEventCounter event_counter_ {std::chrono::seconds {2}};
};

}  // namespace logicsim

#endif