#ifndef LOGIKSIM_SIMULATION_H
#define LOGIKSIM_SIMULATION_H

#include "algorithms.h"
#include "circuit.h"
#include "exceptions.h"

#include <boost/container/small_vector.hpp>
#include <boost/container/vector.hpp>
#include <fmt/format.h>
#include <range/v3/all.hpp>

#include <cmath>
#include <iostream>
#include <ostream>
#include <queue>
#include <string>
#include <type_traits>

/// Done Features
// * delays for each output, needed for wires

/// New Features
// * add timeout to advance simulations
// * flip flops, which requires access to the last state
// * store transition times for wires so they can be drawn
// * negation on input and outputs
// * clock generators
// * shift registers, requires memory & internal state

namespace logicsim {

using time_t = double;  // TODO try float

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

    std::string format() const;
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
using event_group_t = boost::container::small_vector<SimulationEvent, 1>;
void validate(const event_group_t &events);

class SimulationQueue {
   public:
    time_t time() const noexcept;
    time_t next_event_time() const noexcept;
    bool empty() const noexcept;

    void set_time(time_t time);
    void submit_event(SimulationEvent &&event);
    /// Remove and return all events for the next time and element_id.
    event_group_t pop_event_group();

   private:
    time_t time_ {0};
    std::priority_queue<SimulationEvent, std::vector<SimulationEvent>, std::greater<>> events_;
};

/// Represents multiple logic values
using logic_vector_t = boost::container::vector<bool>;
using delay_vector_t = boost::container::vector<time_t>;

using logic_small_vector_t = boost::container::small_vector<bool, 8>;

/// Store simulation data.
struct SimulationState {
    logic_vector_t input_values {};
    SimulationQueue queue {};
    delay_vector_t output_delays {};

    SimulationState(connection_id_t total_inputs, connection_id_t total_outputs);
    SimulationState(const Circuit &circuit);
};

void check_input_size(const SimulationState &state, const Circuit &circuit);

void initialize_simulation(SimulationState &state, const Circuit &circuit);

SimulationState get_initialized_state(Circuit &circuit);

/// @brief Advance the simulation by changing the given simulations state
/// @param state          either new or the old simulation state to start from
/// @param circuit        the circuit that should be simulated
/// @param time_delta     tun for this time or, when zero, run until
///                       no more new events are generated
/// @param print_events   if true print each processed event information
void advance_simulation(SimulationState &state, const Circuit &circuit, time_t time_delta = 0,
                        bool print_events = false);

SimulationState simulate_circuit(Circuit &circuit, time_t time_delta = 0,
                                 bool print_events = false);

bool get_input_value(const Circuit::ConstInput input, const logic_vector_t &input_values);
bool get_input_value(const Circuit::ConstInput input, const SimulationState &state);

/// infers the output value from the connected input value, if it exists.
bool get_output_value(const Circuit::ConstOutput output, const logic_vector_t &input_values,
                      const bool raise_missing = true);
bool get_output_value(const Circuit::ConstOutput output, const SimulationState &state,
                      const bool raise_missing = true);

logic_small_vector_t get_input_values(const Circuit::ConstElement element,
                                      const logic_vector_t &input_values);
logic_small_vector_t get_output_values(const Circuit::ConstElement element,
                                       const logic_vector_t &input_values,
                                       const bool raise_missing = true);
logic_small_vector_t get_input_values(const Circuit::ConstElement element,
                                      const SimulationState &state);
logic_small_vector_t get_output_values(const Circuit::ConstElement element,
                                       const SimulationState &state,
                                       const bool raise_missing = true);

/// infer vector of all output values from the circuit.
logic_vector_t get_all_output_values(const logic_vector_t &input_values, const Circuit &circuit,
                                     const bool raise_missing = true);

time_t get_output_delay(const Circuit::ConstOutput output, const delay_vector_t &output_delays);
time_t get_output_delay(const Circuit::ConstOutput output, const SimulationState &state);

void set_output_delay(const Circuit::ConstOutput output, delay_vector_t &output_delays,
                      const time_t delay);
void set_output_delay(const Circuit::ConstOutput output, SimulationState &state,
                      const time_t delay);

int benchmark_simulation(const int n_elements = 100, bool print = false);

}  // namespace logicsim

#endif
