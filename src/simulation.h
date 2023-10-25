#ifndef LOGIKSIM_SIMULATION_H
#define LOGIKSIM_SIMULATION_H

#include "component/simulation/history_buffer.h"
#include "component/simulation/simulation_queue.h"
#include "format/struct.h"
#include "schematic.h"
#include "timeout_timer.h"
#include "vocabulary/print_events.h"

#include <string>

namespace logicsim {

struct input_t;
struct output_t;
struct internal_state_t;

namespace simulation {
class HistoryView;
class SimulationEventGroup;
}  // namespace simulation

namespace simulation {
/**
 * brief:: Realtime simulation timeout
 */
using realtime_timeout_t = TimeoutTimer::timeout_t;

namespace defaults {
constexpr static auto no_realtime_timeout = TimeoutTimer::defaults::no_timeout;

constexpr static auto infinite_simulation_time = delay_t::max();

constexpr static int64_t no_max_events {0};
};  // namespace defaults

}  // namespace simulation

/**
 * @brief Event driven simulation of schematics.
 *
 * Supported Features
 *      + separate delays for each output, which is needed for wires
 *      + realtime timeout for run, so it an be integrated in GUI
 *      + transition history for wires
 *      + elements have internal state to support sequential logic
 *      + negation on input are fully simulated
 *      + complex logic, clock generators, can be modeled by looping connections
 *        and writing VHDL slide logic
 *
 * Class invariants:
 *      + Schematic is not changed ever.
 *      + Vectors have same size as given schematic:
 *           input_values_, internal_states_, first_input_histories_
 *      + Sub-vectors have size of inputs / internal states.
 *           input_values_, internal_states_
 *      + Internal state size is never changed
 *      + Simulation time is never decreased (promised by SimulationQueue)
 *      + the event queue never contains two events for the same time and input
 *        (required by SimulationQueue)
 *      + Event count never decreases.
 */
class Simulation {
   public:
    using event_count_t = int64_t;

    /**
     * @brief: Creates and initializes the simulation from the schematic.
     */
    [[nodiscard]] explicit Simulation(Schematic &&schematic,
                                      PrintEvents do_print = PrintEvents::no);
    [[nodiscard]] auto time() const noexcept -> time_t;
    [[nodiscard]] auto processed_event_count() const noexcept -> event_count_t;
    /**
     * @brief: Check if simulation is finished.
     */
    [[nodiscard]] auto is_finished() const -> bool;
    [[nodiscard]] auto schematic() const noexcept -> const Schematic &;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto format_element(element_id_t element_id) const -> std::string;

    /**
     * @brief: Runs simulation for given times and events
     *
     * @param simulation_time   simulate for this time or, when run_until_steady,
     *                          run until no more new events are generated
     * @param realtime_timeout  stop if simulation takes longer than this in real-time
     * @param max_events        return after simulating this many events
     */
    auto run(delay_t simulation_time = simulation::defaults::infinite_simulation_time,
             simulation::realtime_timeout_t timeout =
                 simulation::defaults::no_realtime_timeout,
             int64_t max_events = simulation::defaults::no_max_events) -> void;

    // element information
    [[nodiscard]] auto input_values(element_id_t element_id) const
        -> const logic_small_vector_t &;
    [[nodiscard]] auto output_values(element_id_t element_id) const
        -> logic_small_vector_t;
    [[nodiscard]] auto internal_state(element_id_t element_id) const
        -> const logic_small_vector_t &;
    [[nodiscard]] auto input_history(element_id_t element_id) const
        -> simulation::HistoryView;

    [[nodiscard]] auto input_value(input_t input) const -> bool;
    [[nodiscard]] auto output_value(output_t output) const -> bool;
    [[nodiscard]] auto internal_state(internal_state_t index) const -> bool;

    /**
     * @brief: sets the value of an unconnected input.
     *
     * Throws an exception if the input is is connected.
     *
     * Note if the value is different, the simulation advances by 1ns.
     */
    auto set_unconnected_input(input_t input, bool value) -> void;
    /**
     * @brief: tries to set the internal state in the next few time-points
     *
     * The simulation is advanced by 1ns until a time-point is found where
     * the internal state is not changed. Note this can fail, if there is
     * constant input activity.
     *
     * Throws if the elements internal state is not writable, like for clock generators.
     *
     * Returns true if state was successfully changed.
     */
    auto try_set_internal_state(internal_state_t index, bool value) -> bool;

   private:
    auto resize_vectors() -> void;
    auto schedule_initial_events() -> void;
    auto submit_events_for_changed_outputs(element_id_t element_id,
                                           const logic_small_vector_t &old_outputs,
                                           const logic_small_vector_t &new_outputs)
        -> void;
    auto process_all_current_events() -> void;
    auto process_event_group(simulation::SimulationEventGroup &&events) -> void;
    auto submit_event(output_t output, const logic_small_vector_t &output_values) -> void;
    auto apply_events(element_id_t element_id,
                      const simulation::SimulationEventGroup &group) -> void;
    auto set_input_internal(input_t input, bool value) -> void;

    auto record_input_history(input_t input, bool new_value) -> void;
    auto clean_history(simulation::HistoryBuffer &history, delay_t history_length)
        -> void;

    Schematic schematic_;
    simulation::SimulationQueue queue_;
    time_t largest_history_event_;
    bool print_events_;
    event_count_t event_count_;

    std::vector<logic_small_vector_t> input_values_ {};
    std::vector<logic_small_vector_t> internal_states_ {};
    std::vector<simulation::HistoryBuffer> first_input_histories_ {};
};

}  // namespace logicsim

#endif
