#ifndef LOGIKSIM_SIMULATION_H
#define LOGIKSIM_SIMULATION_H

#include "component/simulation/history_buffer.h"
#include "component/simulation/simulation_queue.h"
#include "schematic_old.h"
#include "timeout_timer.h"
#include "vocabulary/print_events.h"

#include <folly/small_vector.h>

/// Done Features
// * delays for each output, needed for wires
// * add timeout to run simulations
// * use discrete integer type for time, like chrono::sim_time<int64_t>
// * store transition times for wires, so they can be drawn
// * flip flops, which requires internal state and access to last input values
// * negation on input and outputs
// * store history of events so lines can be drawn
// * shift registers, requires more internal state
// * clock generators

/// New Features

namespace logicsim {

namespace simulation {
class HistoryView;
}

namespace simulation {
/**
 * brief:: Realtime simulation timeout
 */
using timeout_t = TimeoutTimer::timeout_t;

namespace defaults {
constexpr static auto no_timeout = TimeoutTimer::defaults::no_timeout;
constexpr static auto infinite_simulation_time = delay_t::max();
constexpr static int64_t no_max_events {std::numeric_limits<int64_t>::max() -
                                        connection_count_t::max().count()};
};  // namespace defaults
}  // namespace simulation

class Simulation {
   public:
    [[nodiscard]] explicit Simulation(const SchematicOld &schematic,
                                      PrintEvents print_events = PrintEvents::no);

    [[nodiscard]] auto schematic() const noexcept -> const SchematicOld &;
    [[nodiscard]] auto time() const noexcept -> time_t;

    // submit custom events
    auto submit_event(SchematicOld::ConstInput input, delay_t offset, bool value) -> void;
    auto submit_events(SchematicOld::ConstElement element, delay_t offset,
                       logic_small_vector_t values) -> void;

    // Initialize logic elements in the simulation
    auto initialize() -> void;
    [[nodiscard]] auto is_initialized() const -> bool;

    /**
     * @brief: Runs simulation for given times and events
     *
     * @param simulation_time   simulate for this time or, when run_until_steady,
     *                          run until no more new events are generated
     * @param timeout           return if simulation takes longer than this in realtime
     * @param max_events        return after simulating this many events
     */
    auto run(delay_t simulation_time = simulation::defaults::infinite_simulation_time,
             simulation::timeout_t timeout = simulation::defaults::no_timeout,
             int64_t max_events = simulation::defaults::no_max_events) -> int64_t;
    /**
     * @brief: Runs simulation for a very short time
     */
    auto run_infinitesimal() -> int64_t;
    /**
     * @brief: Check if simulation is finished.
     */
    auto finished() const -> bool;

    // input values
    auto set_input_value(SchematicOld::ConstInput input, bool value) -> void;
    [[nodiscard]] auto input_value(element_id_t element_id, connection_id_t index) const
        -> bool;
    [[nodiscard]] auto input_value(SchematicOld::ConstInput input) const -> bool;
    [[nodiscard]] auto input_values(element_id_t element_id) const
        -> const logic_small_vector_t &;
    [[nodiscard]] auto input_values(SchematicOld::Element element) const
        -> const logic_small_vector_t &;
    [[nodiscard]] auto input_values(SchematicOld::ConstElement element) const
        -> const logic_small_vector_t &;

    // infers the output values
    auto set_output_value(SchematicOld::ConstOutput output, bool value) -> void;
    [[nodiscard]] auto output_value(element_id_t element_id, connection_id_t index) const
        -> bool;
    [[nodiscard]] auto output_value(SchematicOld::ConstOutput output) const -> bool;
    [[nodiscard]] auto output_values(element_id_t element_id) const
        -> logic_small_vector_t;
    [[nodiscard]] auto output_values(SchematicOld::Element element) const
        -> logic_small_vector_t;
    [[nodiscard]] auto output_values(SchematicOld::ConstElement element) const
        -> logic_small_vector_t;

    // internal states
    auto set_internal_state(SchematicOld::ConstElement element, std::size_t index,
                            bool value) -> void;
    [[nodiscard]] auto internal_state(element_id_t element_id) const
        -> const logic_small_vector_t &;
    [[nodiscard]] auto internal_state(SchematicOld::Element element) const
        -> const logic_small_vector_t &;
    [[nodiscard]] auto internal_state(SchematicOld::ConstElement element) const
        -> const logic_small_vector_t &;
    [[nodiscard]] auto internal_state(SchematicOld::ConstElement element,
                                      std::size_t index) const -> bool;

    // history
    auto input_history(element_id_t element_id) const -> simulation::HistoryView;
    auto input_history(SchematicOld::Element element) const -> simulation::HistoryView;
    auto input_history(SchematicOld::ConstElement element) const
        -> simulation::HistoryView;

   private:
    auto check_counts_valid() const -> void;

    auto submit_events_for_changed_outputs(const SchematicOld::ConstElement element,
                                           const logic_small_vector_t &old_outputs,
                                           const logic_small_vector_t &new_outputs)
        -> void;
    auto process_event_group(simulation::event_group_t &&events) -> void;
    auto create_event(SchematicOld::ConstOutput output,
                      const logic_small_vector_t &output_values) -> void;
    auto apply_events(SchematicOld::ConstElement element,
                      const simulation::event_group_t &group) -> void;
    auto set_input_internal(SchematicOld::ConstInput input, bool value) -> void;

    auto record_input_history(SchematicOld::ConstInput input, bool new_value) -> void;
    auto clean_history(simulation::history_buffer_t &history, delay_t history_length)
        -> void;

    gsl::not_null<const SchematicOld *> schematic_;
    simulation::SimulationQueue queue_;
    time_t largest_history_event_;
    bool is_initialized_;
    bool print_events_;

    std::vector<logic_small_vector_t> input_values_ {};
    std::vector<logic_small_vector_t> internal_states_ {};
    std::vector<simulation::history_buffer_t> first_input_histories_ {};
};

auto set_default_outputs(Simulation &simulation) -> void;
auto set_default_inputs(Simulation &simulation) -> void;

}  // namespace logicsim

#endif
