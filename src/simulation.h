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

    bool operator==(const SimulationEvent &other) const;
    bool operator<(const SimulationEvent &other) const;

    bool operator!=(const SimulationEvent &other) const;
    bool operator>(const SimulationEvent &other) const;
    bool operator<=(const SimulationEvent &other) const;
    bool operator>=(const SimulationEvent &other) const;

    [[nodiscard]] std::string format() const;
};

static_assert(std::is_trivial<SimulationEvent>::value);
static_assert(std::is_trivially_copyable<SimulationEvent>::value);
static_assert(std::is_standard_layout<SimulationEvent>::value);

SimulationEvent make_event(Circuit::ConstInput input, time_t time, bool value);
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
std::basic_ostream<CharT> &operator<<(std::basic_ostream<CharT> &os,
                                      const logicsim::SimulationEvent &dt) {
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
    [[nodiscard]] time_t time() const noexcept;
    [[nodiscard]] time_t next_event_time() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

    void set_time(time_t time);
    void submit_event(SimulationEvent event);
    /// Remove and return all events for the next time and element_id.
    event_group_t pop_event_group();

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

[[nodiscard]] SimulationState get_uninitialized_state(Circuit &circuit);
[[nodiscard]] SimulationState get_initialized_state(Circuit &circuit);

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
int64_t advance_simulation(SimulationState &state, const Circuit &circuit,
                           time_t simultation_time = defaults::infinite_simulation_time,
                           timeout_t timeout = defaults::no_timeout,
                           int64_t max_events = defaults::no_max_events,
                           bool print_events = false);

[[nodiscard]] SimulationState simulate_circuit(Circuit &circuit,
                                               time_t simultation_time
                                               = defaults::infinite_simulation_time,
                                               timeout_t timeout = defaults::no_timeout,
                                               bool print_events = false);

[[nodiscard]] bool get_input_value(Circuit::ConstInput input,
                                   const logic_vector_t &input_values);
[[nodiscard]] bool get_input_value(Circuit::ConstInput input,
                                   const SimulationState &state);

/// infers the output value from the connected input value, if it exists.
[[nodiscard]] bool get_output_value(Circuit::ConstOutput output,
                                    const logic_vector_t &input_values,
                                    bool raise_missing = true);
[[nodiscard]] bool get_output_value(Circuit::ConstOutput output,
                                    const SimulationState &state,
                                    bool raise_missing = true);

[[nodiscard]] logic_small_vector_t get_input_values(Circuit::ConstElement element,
                                                    const logic_vector_t &input_values);
[[nodiscard]] logic_small_vector_t get_output_values(Circuit::ConstElement element,
                                                     const logic_vector_t &input_values,
                                                     bool raise_missing = true);
[[nodiscard]] logic_small_vector_t get_input_values(Circuit::ConstElement element,
                                                    const SimulationState &state);
[[nodiscard]] logic_small_vector_t get_output_values(Circuit::ConstElement element,
                                                     const SimulationState &state,
                                                     bool raise_missing = true);

/// infer vector of all output values from the circuit.
[[nodiscard]] logic_vector_t get_all_output_values(const logic_vector_t &input_values,
                                                   const Circuit &circuit,
                                                   bool raise_missing = true);

[[nodiscard]] time_t get_output_delay(Circuit::ConstOutput output,
                                      const delay_vector_t &output_delays);
[[nodiscard]] time_t get_output_delay(Circuit::ConstOutput output,
                                      const SimulationState &state);

void set_output_delay(Circuit::ConstOutput output, SimulationState &state, time_t delay);

template <std::uniform_random_bit_generator G>
auto benchmark_simulation(G &rng, const Circuit &circuit, const int n_events,
                          const bool print) -> int64_t;
auto benchmark_simulation(int n_elements = 100, int n_events = 10'000, bool print = false)
    -> int64_t;

}  // namespace logicsim

#endif
