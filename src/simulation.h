#ifndef LOGIKSIM_SIMULATION_H
#define LOGIKSIM_SIMULATION_H

#include "circuit.h"

#include <boost/container/small_vector.hpp>
#include <boost/container/vector.hpp>
#include <fmt/core.h>

#include <chrono>
#include <ostream>
#include <queue>
#include <random>
#include <string>

/// Done Features
// * delays for each output, needed for wires
// * add timeout to advance simulations

/// New Features
// * use discrete integer type for time, like chronos::sim_time<int64_t>
// * flip flops, which requires access to the last state
// * store transition times for wires so they can be drawn
// * negation on input and outputs
// * clock generators
// * shift registers, requires memory & internal state

namespace logicsim {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4455)  // literal suffix identifiers are reserved
#endif
using std::literals::chrono_literals::operator""us;
using std::literals::chrono_literals::operator""ns;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

// TODO strong type for time_t
using time_t = std::chrono::duration<int64_t, std::nano>;

struct SimulationEvent {
    time_t time;
    element_id_t element_id;
    connection_size_t input_index;
    bool value;

    auto operator==(const SimulationEvent &other) const -> bool;
    auto operator<(const SimulationEvent &other) const -> bool;

    auto operator!=(const SimulationEvent &other) const -> bool;
    auto operator>(const SimulationEvent &other) const -> bool;
    auto operator<=(const SimulationEvent &other) const -> bool;
    auto operator>=(const SimulationEvent &other) const -> bool;

    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::is_trivial<SimulationEvent>::value);
static_assert(std::is_trivially_copyable<SimulationEvent>::value);
static_assert(std::is_standard_layout<SimulationEvent>::value);

auto make_event(Circuit::ConstInput input, time_t time, bool value) -> SimulationEvent;
}  // namespace logicsim

template <>
struct fmt::formatter<logicsim::SimulationEvent> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const logicsim::SimulationEvent &obj, fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

namespace std {

template <class CharT>
auto operator<<(std::basic_ostream<CharT> &os, const logicsim::SimulationEvent &dt)
    -> std::basic_ostream<CharT> & {
    os << dt.format();
    return os;
}
}  // namespace std

namespace logicsim {

/// groups of events for the same element and time  but different inputs
using event_group_t = boost::container::small_vector<SimulationEvent, 2>;
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

/// Represents multiple logic values
using logic_vector_t = boost::container::vector<bool>;
using delay_vector_t = boost::container::vector<time_t>;

// 8 bytes still fit into a small_vector with 32 byte size.
using logic_small_vector_t = boost::container::small_vector<bool, 8>;

/// Store simulation data.
struct SimulationState {
    constexpr static time_t standard_delay = 100us;

    logic_vector_t input_values {};
    SimulationQueue queue {};
    delay_vector_t output_delays {};

    explicit SimulationState(connection_id_t total_inputs, connection_id_t total_outputs);
    explicit SimulationState(const Circuit &circuit);
};

void check_input_size(const SimulationState &state, const Circuit &circuit);

void initialize_simulation(SimulationState &state, const Circuit &circuit);

[[nodiscard]] auto get_uninitialized_state(Circuit &circuit) -> SimulationState;
[[nodiscard]] auto get_initialized_state(Circuit &circuit) -> SimulationState;

using timeout_clock = std::chrono::steady_clock;
using timeout_t = timeout_clock::duration;

namespace defaults {
constexpr auto no_timeout = timeout_t::max();
constexpr auto infinite_simulation_time = time_t::max();
constexpr int64_t no_max_events
    = std::numeric_limits<int64_t>::max() - std::numeric_limits<connection_size_t>::max();
}  // namespace defaults

/// @brief Advance the simulation by changing the given simulations state
/// @param state             either new or the old simulation state to start from
/// @param circuit           the circuit that should be simulated
/// @param simultation_time  simulate for this time or, when run_until_steady, run until
///                          no more new events are generated
/// @param timeout           return if simulation takes longer than this in realtime
/// @param print_events      if true print each processed event information
auto advance_simulation(SimulationState &state, const Circuit &circuit,
                        time_t simultation_time = defaults::infinite_simulation_time,
                        timeout_t timeout = defaults::no_timeout,
                        int64_t max_events = defaults::no_max_events,
                        bool print_events = false) -> int64_t;

[[nodiscard]] auto simulate_circuit(Circuit &circuit,
                                    time_t simultation_time
                                    = defaults::infinite_simulation_time,
                                    timeout_t timeout = defaults::no_timeout,
                                    bool print_events = false) -> SimulationState;

[[nodiscard]] auto get_input_value(Circuit::ConstInput input,
                                   const logic_vector_t &input_values) -> bool;
[[nodiscard]] auto get_input_value(Circuit::ConstInput input,
                                   const SimulationState &state) -> bool;

/// infers the output value from the connected input value, if it exists.
[[nodiscard]] auto get_output_value(Circuit::ConstOutput output,
                                    const logic_vector_t &input_values,
                                    bool raise_missing = true) -> bool;
[[nodiscard]] auto get_output_value(Circuit::ConstOutput output,
                                    const SimulationState &state,
                                    bool raise_missing = true) -> bool;

[[nodiscard]] auto get_input_values(Circuit::ConstElement element,
                                    const logic_vector_t &input_values)
    -> logic_small_vector_t;
[[nodiscard]] auto get_output_values(Circuit::ConstElement element,
                                     const logic_vector_t &input_values,
                                     bool raise_missing = true) -> logic_small_vector_t;
[[nodiscard]] auto get_input_values(Circuit::ConstElement element,
                                    const SimulationState &state) -> logic_small_vector_t;
[[nodiscard]] auto get_output_values(Circuit::ConstElement element,
                                     const SimulationState &state,
                                     bool raise_missing = true) -> logic_small_vector_t;

/// infer vector of all output values from the circuit.
[[nodiscard]] auto get_all_output_values(const logic_vector_t &input_values,
                                         const Circuit &circuit,
                                         bool raise_missing = true) -> logic_vector_t;

[[nodiscard]] auto get_output_delay(Circuit::ConstOutput output,
                                    const delay_vector_t &output_delays) -> time_t;
[[nodiscard]] auto get_output_delay(Circuit::ConstOutput output,
                                    const SimulationState &state) -> time_t;

void set_output_delay(Circuit::ConstOutput output, SimulationState &state, time_t delay);

inline constexpr int BENCHMARK_DEFAULT_EVENTS {10'000};

template <std::uniform_random_bit_generator G>
auto benchmark_simulation(G &rng, const Circuit &circuit, const int n_events,
                          const bool print) -> int64_t;
auto benchmark_simulation(int n_elements = BENCHMARK_DEFAULT_ELEMENTS,
                          int n_events = BENCHMARK_DEFAULT_EVENTS, bool print = false)
    -> int64_t;

}  // namespace logicsim

#endif
