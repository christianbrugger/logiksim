#ifndef LOGIKSIM_SIMULATION_H
#define LOGIKSIM_SIMULATION_H

#include "circular_buffer.h"
#include "exceptions.h"
#include "schematic.h"

#include <boost/container/small_vector.hpp>
#include <boost/container/vector.hpp>
#include <fmt/core.h>
#include <folly/small_vector.h>
#include <gsl/gsl>

#include <chrono>
#include <ostream>
#include <queue>
#include <random>
#include <string>
#include <type_traits>

/// Done Features
// * delays for each output, needed for wires
// * add timeout to run simulations
// * use discrete integer type for time, like chronos::sim_time<int64_t>
// * store transition times for wires, so they can be drawn
// * flip flops, which requires internal state and access to last input values
// * negation on input and outputs
// * store history of events so lines can be drawn
// * shift registers, requires more internal state
// * clock generators

/// New Features

namespace logicsim {

// TODO packing
struct SimulationEvent {
    time_t time;
    element_id_t element_id;
    connection_id_t input_index;
    bool value;

    auto operator==(const SimulationEvent &other) const -> bool;
    auto operator<(const SimulationEvent &other) const -> bool;

    auto operator!=(const SimulationEvent &other) const -> bool;
    auto operator>(const SimulationEvent &other) const -> bool;
    auto operator<=(const SimulationEvent &other) const -> bool;
    auto operator>=(const SimulationEvent &other) const -> bool;

    [[nodiscard]] auto format() const -> std::string;
};

static_assert(sizeof(SimulationEvent) == 16);

static_assert(std::is_trivial<SimulationEvent>::value);
static_assert(std::is_trivially_copyable<SimulationEvent>::value);
static_assert(std::is_standard_layout<SimulationEvent>::value);

auto make_event(Schematic::ConstInput input, time_t time, bool value) -> SimulationEvent;

/// groups of events for the same element and time  but different inputs
// using event_group_t = boost::container::small_vector<SimulationEvent, 2>;
using event_group_t = folly::small_vector<SimulationEvent, 4>;
void validate(const event_group_t &events);

class SimulationQueue {
   public:
    [[nodiscard]] auto time() const noexcept -> time_t;
    [[nodiscard]] auto next_event_time() const noexcept -> time_t;
    [[nodiscard]] auto empty() const noexcept -> bool;

    void set_time(time_t time);
    void submit_event(SimulationEvent event);
    /// Remove and return all events for the next time and element_id.
    auto pop_event_group() -> event_group_t;

   private:
    time_t time_ {0us};
    std::priority_queue<SimulationEvent, std::vector<SimulationEvent>, std::greater<>>
        events_;
};

using timeout_clock = std::chrono::steady_clock;
using timeout_t = timeout_clock::duration;

class Simulation {
   private:
    struct ElementState;

   public:
    // Remove and put into run parameters
    bool print_events {false};

    /// Represents multiple logic values
    using logic_vector_t = boost::container::vector<bool>;

    using history_vector_t = circular_buffer<time_t, 2, uint32_t>;

    // 8 bytes still fit into a small_vector with 32 byte size.
    // using logic_small_vector_t = boost::container::small_vector<bool, 8>;
    // using con_index_small_vector_t = boost::container::small_vector<connection_id_t,
    // 8>;

    using policy = folly::small_vector_policy::policy_size_type<uint32_t>;
    using logic_small_vector_t = folly::small_vector<bool, 20, policy>;
    using con_index_small_vector_t = folly::small_vector<connection_id_t, 20, policy>;
    static_assert(sizeof(logic_small_vector_t) == 24);
    static_assert(sizeof(con_index_small_vector_t) == 24);

    // TODO remove, now part of editable circuit
    constexpr static delay_t wire_delay_per_distance {10us};

    struct defaults {
        constexpr static delay_t standard_delay {100us};

        constexpr static auto no_timeout = timeout_t::max();
        constexpr static auto infinite_simulation_time = time_t::value_type::max();
        constexpr static int64_t no_max_events {std::numeric_limits<int64_t>::max()
                                                - connection_id_t::max()};

        constexpr static delay_t no_history {0ns};
    };

   public:
    [[nodiscard]] explicit Simulation(const Schematic &schematic);
    [[nodiscard]] auto schematic() const noexcept -> const Schematic &;
    [[nodiscard]] auto time() const noexcept -> time_t;

    // submit custom events
    auto submit_event(Schematic::ConstInput input, time_t::value_type offset, bool value)
        -> void;
    auto submit_events(Schematic::ConstElement element, time_t::value_type offset,
                       logic_small_vector_t values) -> void;

    // Initialize logic elements in the simulation
    auto initialize() -> void;

    /// @brief Run the simulation by changing the given simulations state
    /// @param simulation_time   simulate for this time or, when run_until_steady,
    ///                          run until no more new events are generated
    /// @param timeout           return if simulation takes longer than this in realtime
    /// @param max_events        return after simulating this many events
    auto run(time_t::value_type simulation_time = defaults::infinite_simulation_time,
             timeout_t timeout = defaults::no_timeout,
             int64_t max_events = defaults::no_max_events) -> int64_t;
    // After any events are submitted or function is called that submits events, like
    // set_internal_state, the simulation needs to be advanced in a small way.
    // Otherwise there might be two events at the same time for the same input leading
    // to a runtime error.
    auto run_infinitesimal() -> int64_t;

    // input values
    [[nodiscard]] auto input_value(element_id_t element_id, connection_id_t index) const
        -> bool;
    [[nodiscard]] auto input_value(Schematic::ConstInput input) const -> bool;
    [[nodiscard]] auto input_values(Schematic::ConstElement element) const
        -> logic_small_vector_t;
    [[nodiscard]] auto input_values() const -> const logic_vector_t;

    // infers the output values
    [[nodiscard]] auto output_value(Schematic::ConstOutput output,
                                    bool raise_missing = true) const -> bool;
    [[nodiscard]] auto output_values(Schematic::ConstElement element,
                                     bool raise_missing = true) const
        -> logic_small_vector_t;
    [[nodiscard]] auto output_values(bool raise_missing = true) const -> logic_vector_t;

    // inverters
    [[nodiscard]] auto input_inverter(Schematic::ConstInput input) const -> bool;
    [[nodiscard]] auto input_inverters(Schematic::ConstElement element) const
        -> logic_small_vector_t;
    auto set_input_inverter(Schematic::ConstInput input, bool value) -> void;
    auto set_input_inverters(Schematic::ConstElement element, logic_small_vector_t values)
        -> void;

    // delays
    auto set_output_delay(Schematic::ConstOutput output, delay_t delay) -> void;
    auto set_output_delays(Schematic::ConstElement element,
                           std::span<const delay_t> delays) -> void;
    [[nodiscard]] auto output_delay(Schematic::ConstOutput output) const -> delay_t;

    // internal states
    auto set_internal_state(Schematic::ConstElement element, std::size_t index,
                            bool value) -> void;
    [[nodiscard]] auto internal_state(Schematic::ConstElement element) const
        -> const logic_small_vector_t &;
    [[nodiscard]] auto internal_state(Schematic::ConstElement element,
                                      std::size_t index) const -> bool;

    // history
    class HistoryView;
    class HistoryIterator;
    struct history_entry_t;
    auto set_history_length(Schematic::ConstElement element, delay_t history_length)
        -> void;
    auto history_length(Schematic::ConstElement element) const -> delay_t;
    auto input_history(Schematic::ConstElement element) const -> HistoryView;

   private:
    class Timer;

    auto check_state_valid() const -> void;

    auto submit_events_for_changed_outputs(const Schematic::ConstElement element,
                                           const logic_small_vector_t &old_outputs,
                                           const logic_small_vector_t &new_outputs)
        -> void;
    auto process_event_group(event_group_t &&events) -> void;
    auto create_event(Schematic::ConstOutput output,
                      const logic_small_vector_t &output_values) -> void;
    auto apply_events(Schematic::ConstElement element, const event_group_t &group)
        -> void;
    auto set_input(Schematic::ConstInput input, bool value) -> void;
    // auto set_internal_state(Schematic::ConstElement element,
    //                         const logic_small_vector_t &state) -> void;

    auto record_input_history(Schematic::ConstInput input, bool new_value) -> void;
    auto clean_history(history_vector_t &history, delay_t history_length) -> void;

    [[nodiscard]] auto get_state(element_id_t element_id) -> ElementState &;
    [[nodiscard]] auto get_state(element_id_t element_id) const -> const ElementState &;
    [[nodiscard]] auto get_state(ElementOrConnection auto item) -> ElementState &;
    [[nodiscard]] auto get_state(ElementOrConnection auto item) const
        -> const ElementState &;

    // TODO use schematic values instead of local ones, then delete
    // TODO use vectors instead of struct
    struct ElementState {
        logic_small_vector_t input_values {};
        logic_small_vector_t input_inverters {};
        logic_small_vector_t internal_state {};

        using policy = folly::small_vector_policy::policy_size_type<uint32_t>;
        folly::small_vector<delay_t, 5, policy> output_delays {};

        history_vector_t first_input_history {};
        delay_t history_length {defaults::no_history};

        static_assert(sizeof(first_input_history) == 28);
        static_assert(sizeof(output_delays) == 24);
    };

    static_assert(sizeof(ElementState) == 128);

    gsl::not_null<const Schematic *> schematic_;
    std::vector<ElementState> states_ {};
    SimulationQueue queue_ {};
    bool is_initialized_ {false};
};

class Simulation::HistoryView {
    friend HistoryIterator;

   public:
    using iterator_type = HistoryIterator;

    using value_type = history_entry_t;
    using pointer = history_entry_t;
    using reference = history_entry_t;

   public:
    [[nodiscard]] explicit HistoryView() = default;
    [[nodiscard]] explicit HistoryView(const history_vector_t &history,
                                       time_t simulation_time, bool last_value,
                                       delay_t history_length);

    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto ssize() const -> std::ptrdiff_t;

    [[nodiscard]] auto begin() const -> HistoryIterator;
    [[nodiscard]] auto end() const -> HistoryIterator;

    [[nodiscard]] auto from(time_t value) const -> HistoryIterator;
    [[nodiscard]] auto until(time_t value) const -> HistoryIterator;

    [[nodiscard]] auto value(time_t value) const -> bool;

   private:
    auto require_history() const -> void;

    [[nodiscard]] auto get_value(std::size_t history_index) const -> bool;
    [[nodiscard]] auto find_index(time_t value) const -> std::size_t;
    [[nodiscard]] auto get_time(std::ptrdiff_t index,
                                bool substract_epsilon = false) const -> time_t;

   private:
    const history_vector_t *history_ {nullptr};
    time_t simulation_time_ {};
    history_vector_t::internal_size_t min_index_ {};
    bool last_value_ {};
};

struct Simulation::history_entry_t {
    time_t first_time;
    time_t last_time;
    bool value;

    auto format() const -> std::string;
    auto operator==(const history_entry_t &other) const -> bool = default;
};

class Simulation::HistoryIterator {
   public:
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;

    using value_type = history_entry_t;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type;
    using reference = value_type;

    [[nodiscard]] explicit HistoryIterator() = default;
    [[nodiscard]] explicit HistoryIterator(HistoryView view, std::size_t index) noexcept;

    [[nodiscard]] auto operator*() const -> value_type;
    auto operator++() noexcept -> HistoryIterator &;
    auto operator++(int) noexcept -> HistoryIterator;

    [[nodiscard]] auto operator==(const HistoryIterator &right) const noexcept -> bool;
    [[nodiscard]] auto operator-(const HistoryIterator &right) const noexcept
        -> difference_type;

   private:
    HistoryView view_ {};
    // from 0 to history.size() + 1
    std::size_t index_ {};
};
}  // namespace logicsim

static_assert(std::forward_iterator<logicsim::Simulation::HistoryIterator>);

template <>
inline constexpr bool std::ranges::enable_view<logicsim::Simulation::HistoryView> = true;

// benchmark

namespace logicsim {

constexpr int BENCHMARK_DEFAULT_EVENTS {10'000};

template <std::uniform_random_bit_generator G>
auto benchmark_simulation(G &rng, const Schematic &schematic, const int n_events,
                          const bool do_print) -> int64_t;
auto benchmark_simulation(int n_elements = BENCHMARK_DEFAULT_ELEMENTS,
                          int n_events = BENCHMARK_DEFAULT_EVENTS, bool do_print = false)
    -> int64_t;

}  // namespace logicsim

#endif
