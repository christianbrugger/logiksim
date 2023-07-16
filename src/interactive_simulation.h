#ifndef LOGIKSIM_INTERACTIVE_SIMULATION_H
#define LOGIKSIM_INTERACTIVE_SIMULATION_H

#include "layout.h"
#include "schematic.h"
#include "simulation.h"
#include "timer.h"
#include "vocabulary.h"

#include <ankerl/unordered_dense.h>

namespace logicsim {

namespace detail::interactive_simulation {

struct iteraction_data_t {
    element_id_t element_id {null_element};

    auto operator==(const iteraction_data_t& other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

class InteractionCache {
   public:
    using map_type = ankerl::unordered_dense::map<point_t, iteraction_data_t>;

   public:
    InteractionCache(const Layout& layout);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto find(point_t position) const -> std::optional<element_id_t>;

   private:
    map_type map_ {};
};

}  // namespace detail::interactive_simulation

class InteractiveSimulation {
    using timer_t = std::chrono::steady_clock;
    using realtime_t = timer_t::time_point;

   public:
    struct defaults {
        constexpr static auto default_timeout = timeout_t {1ms};
    };

   public:
    InteractiveSimulation(const Layout& layout);

    [[nodiscard]] auto schematic() const -> const Schematic&;
    [[nodiscard]] auto simulation() const -> const Simulation&;

    auto set_time_rate(time_rate_t time_rate) -> void;
    [[nodiscard]] auto time_rate() const -> time_rate_t;
    [[nodiscard]] auto time() const -> time_t;

    auto run(timeout_t timeout = defaults::default_timeout) -> int64_t;

    auto mouse_press(point_t position) -> void;

   private:
    [[nodiscard]] auto expected_simulation_time() const -> time_t;

   private:
    Schematic schematic_;
    Simulation simulation_;
    detail::interactive_simulation::InteractionCache interaction_cache_;

    time_t simulation_time_reference_ {};
    realtime_t realtime_reference_ {};

    time_rate_t time_rate_ {50us};
};

}  // namespace logicsim

#endif